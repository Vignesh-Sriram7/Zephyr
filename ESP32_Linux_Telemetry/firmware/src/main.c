#include <zephyr/kernel.h>
// Header for use of I2C bus
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h> // Device configurations
#include <zephyr/drivers/sensor.h> // BME280 implements this API

// Retrieve the defined devices from the devicetree //

static const struct device *const bme280 = DEVICE_DT_GET(DT_ALIAS(my_temp)); // Retrieve the Sensor device

int main(void){

    // Declare the varaibles required for the bme280 sensor
    int ret;
    struct sensor_value temp;
    struct sensor_value hum;
    uint32_t timestamp = 0; 

    /* CHECK IF DEVICES ARE READY */

    if(!device_is_ready(bme280)){
            printk("Device %s is not ready.\n", bme280->name);
            return 0;
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

        ret = sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &hum);
        if(ret<0)
        {
            printk("Channel Get Error: %d\n", ret);
            continue;
        }

        timestamp = k_uptime_get_32();
        // Protocol format: TEMP:XX.XX,HUM:XX.XX,TS:XXXX\n
        printk("TEMP:%d.%02d,HUM:%d.%02d,TS:%u\n", 
                temp.val1, (temp.val2 / 10000), 
                hum.val1, (hum.val2 / 10000), 
                timestamp);
        
        k_msleep(1000);
    }
    
    return 0;
}
        