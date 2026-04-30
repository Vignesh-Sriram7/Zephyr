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
#include <zephyr/drivers/entropy.h>


// Retrieve the Sensor device
static const struct device *const bme280 = DEVICE_DT_GET(DT_ALIAS(my_temp));

// Retrieve the RNG device
static const struct device *const dev_rng = DEVICE_DT_GET(DT_NODELABEL(rng));

// Define the nvs file system structure
static struct nvs_fs fs;

// Set the storage partition as a device due to its use of raw_flash
#define STORAGE_PARTITION FIXED_PARTITION_DEVICE(storage_partition)
static const struct device *const storage_dev = STORAGE_PARTITION;

#define SEQ_NUM_ID 1
static uint32_t current_seq = 0; // Local variable to hold the number

// Define the struct to hold the logging data
struct __attribute__((packed)) log_entry {
    uint32_t seq_num;        // From NVS
    int32_t  temp_mantissa;  // temp.val1
    int32_t  temp_fraction;  // temp.val2
    uint8_t  rng_token[4];   // From Entropy
    uint8_t  mac[16];        // The HMAC signature
};

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

    struct log_entry my_log;

    /* CHECK IF DEVICES ARE READY */

    if(!device_is_ready(bme280)){
            printk("Device %s is not ready.\n", bme280->name);
            return 0;
        }

    if(!device_is_ready(dev_rng)){
            printk("Device %s is not ready.\n", dev_rng->name);
            return 0;
        }



    /* NVS SETUP*/

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
    
    
    rc = nvs_read(&fs, SEQ_NUM_ID, &current_seq, sizeof(current_seq));

    if(rc<=0)
    {
        printk("Error fetching the sequence numeber\n");
    }

    else
        printk("Sequence number fetched %d \n", current_seq);

    while(1){

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
        
        // Get the anti-replay tokens
        entropy_get_entropy(dev_rng, token, sizeof(token));

        // Increment the counter
        current_seq++;

        // Link it to the struct
        my_log.seq_num = current_seq;

        // Save the NEW number back to NVS 
        nvs_write(&fs, SEQ_NUM_ID, &current_seq, sizeof(current_seq));

        printk("Temp: %d.%06d | Token: %02x%02x%02x%02x\n", 
                temp.val1, temp.val2, 
                token[0], token[1], token[2], token[3]);

        k_msleep(1000);
    }

}