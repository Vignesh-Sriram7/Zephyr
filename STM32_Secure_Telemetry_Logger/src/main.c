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

// Define the nvs file system structure
static struct nvs_fs fs;

// Set the storage partition as a device due to its use of raw_flash
#define STORAGE_PARTITION FIXED_PARTITION_DEVICE(storage_partition)
static const struct device *const storage_dev = STORAGE_PARTITION;

int main(void){

int rc = 0;
// To obtain the page size
struct flash_pages_info info;

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
}