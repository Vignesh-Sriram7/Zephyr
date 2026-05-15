#include <zephyr/kernel.h>
// Header for use of I2C bus
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

// Headers for the use of partitioned flash map
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/device.h> // Device configurations
#include <zephyr/drivers/sensor.h> // BME280 implements this API

// To generate anti replay tokens
#include <zephyr/random/random.h>

// Include the APIs of mbedtls
#include <mbedtls/md.h>

// Retrieve the defined devices from the devicetree //

static const struct device *const bme280 = DEVICE_DT_GET(DT_ALIAS(my_temp)); // Retrieve the Sensor device

// Define the nvs file system structure
static struct nvs_fs fs;

// Set the storage partition as a device due to its use of raw_flash
#define STORAGE_PARTITION FIXED_PARTITION_DEVICE(storage_partition)
static const struct device *const storage_dev = STORAGE_PARTITION;

// Define the IDS used for the NVS write later on //

#define SEQ_NUM_ID 1    // Let the ID for the sequence number be 1


#define HMAC_ID 2   // Let the ID for the HMAC be 2


// Local variable to hold the number
static uint32_t current_seq = 0;

// Acts like bookmark
static uint32_t write_offset = 0;

// HMAC key
static uint8_t hmac_key[16];


// Define the struct to hold the logging data
struct __attribute__((packed)) log_entry {
    uint32_t seq_num;        // From NVS
    int32_t  temp_mantissa;  // temp.val1
    int32_t  temp_fraction;  // temp.val2
    uint8_t  rng_token[4];   // From Entropy
    uint8_t  mac[32];        // The HA-256 HMAC is 32 bytes
};

// Function to compute the HMAC security key
static int compute_hmac(struct log_entry *entry, uint8_t *key)
{
    const mbedtls_md_info_t *md_info;

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return -1;
    }

    return mbedtls_md_hmac(
        md_info,
        key, 16,
        (const unsigned char *)entry,
        sizeof(struct log_entry) - sizeof(entry->mac), // ❗ exclude MAC
        entry->mac
    );
}


// Function to verify the HMAC integrity
void verify_log(uint32_t offset)
{
    struct log_entry read_log;
    uint8_t stored_mac[32];

    flash_read(storage_dev, offset, &read_log, sizeof(read_log));

    memcpy(stored_mac, read_log.mac, 32);

    memset(read_log.mac, 0, 32);

    compute_hmac(&read_log, hmac_key);

    if (memcmp(stored_mac, read_log.mac, 32) == 0) {
        printk("Log valid\n");
    } else {
        printk("Log tampered\n");
    }
}

int main(void){

    // Declare the varaibles required for the bme280 sensor
    int ret;
    struct sensor_value temp;

    // Declare the variables required for the NVS setup
    int rc = 0;
    // To obtain the page size
    struct flash_pages_info info;

    // Declare the RNG tokens 32 bit
    uint8_t token[4];

    // Struct to enter the data into the log
    struct log_entry my_log;

    /* CHECK IF DEVICES ARE READY */

    if(!device_is_ready(bme280)){
            printk("Device %s is not ready.\n", bme280->name);
            return 0;
        }


    // NVS setup //

    // Set up the NVS as a file manager
    fs.flash_device = FIXED_PARTITION_DEVICE(nvs_partition);
    if (!device_is_ready(fs.flash_device)) {
        printk("Flash device %s is not ready\n", fs.flash_device->name);
        return 0;
    }
    fs.offset = FIXED_PARTITION_OFFSET(nvs_partition);

    // Obtain flash page info to get the sector size
    rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (rc) {
        printk("Unable to get page info, rc=%d\n", rc);
        return 0;
    }
    fs.sector_size = info.size;
    // Set sector count based on total partition size / total page size
    fs.sector_count = 3U;

    // Mount to booth the file manage for those partitions
    rc = nvs_mount(&fs);
        if (rc) {
            printk("Flash Init failed, rc=%d\n", rc);
            return 0;
        }

    printk("NVS mounted successfully\n");
    
    
    // Read the sequence number from the NVS
    rc = nvs_read(&fs, SEQ_NUM_ID, &current_seq, sizeof(current_seq));

    if (rc < 0) {
        printk("Error reading sequence number\n");
    } else if (rc == 0) {
        printk("No stored sequence number found\n");
        current_seq = 0;
    } else {
        printk("Sequence number fetched %d\n", current_seq);
    }

    // Read the HMAC key from the NVS
    rc = nvs_read(&fs, HMAC_ID, hmac_key, sizeof(hmac_key));

    // If HMAC not found geenrate one and write to the NVS with a different ID
    if (rc <= 0) {
        printk("No key found, generating new one...\n");
        sys_rand_get(hmac_key, sizeof(hmac_key));

        nvs_write(&fs, HMAC_ID, hmac_key, sizeof(hmac_key));
    } else {
        printk("HMAC key loaded from NVS\n");
    }

    while(1){

        // Obtain the sensor readings
        ret = sensor_sample_fetch(bme280);
        if(ret < 0){
            printk("Sample Fetch Error: %d\n", ret);
            continue;
        }

        ret = sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        if(ret < 0){
            printk("Channel Get Error: %d\n", ret);
            continue;
        }
        
        // Get the anti-replay tokens for each sensor data logging
        sys_rand_get(token, sizeof(token));

        // Increment the counter
        current_seq++;

        // Link it to the struct
        my_log.seq_num = current_seq;

        // Save the new number back to NVS 
        nvs_write(&fs, SEQ_NUM_ID, &current_seq, sizeof(current_seq));

        // Load the ttemperature values to the data struct
        my_log.temp_mantissa = temp.val1;
        my_log.temp_fraction = temp.val2;

        memcpy(my_log.rng_token, token, 4);

        memset(my_log.mac, 0, sizeof(my_log.mac));

        // Compute the HMAC using the data struct and the hmac key
        if (compute_hmac(&my_log, hmac_key) != 0) {
            printk("HMAC computation failed\n");
            continue;
        }
        
        // Write the data log into the storage partition
        rc = flash_write(storage_dev, write_offset, &my_log, sizeof(my_log));

        if (rc == 0) {
            printk("Log #%d saved to storage at offset %d\n", current_seq, write_offset);

            // Verify the stored log
            verify_log(write_offset);
            write_offset += sizeof(my_log);

            if (write_offset + sizeof(my_log) > FIXED_PARTITION_SIZE(storage_partition)) {
                printk("Storage full! Restarting from offset 0...\n");
                rc = flash_erase(storage_dev, 0, FIXED_PARTITION_SIZE(storage_partition));

                if (rc != 0) {
                    printk("Flash erase failed: %d\n", rc);
                }
                write_offset = 0;
            }
        } else {
            printk("Error writing to storage: %d\n", rc);
            }


        printk("Temp: %d.%06d | Token: %02x%02x%02x%02x\n", 
                temp.val1, temp.val2, 
                token[0], token[1], token[2], token[3]);

        k_msleep(1000);
    }


}