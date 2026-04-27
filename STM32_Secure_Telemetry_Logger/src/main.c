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

#define STORAGE_PARTITION FIXED_PARTITION_DEVICE(storage_partition)
static const struct device *const storage_dev = STORAGE_PARTITION;
