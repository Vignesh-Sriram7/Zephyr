#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
//#include <zephyr/device.h> // Device configurations
//#include <zephyr/drivers/sensor.h> // BME280 implements this API

// Retrieve the defined devices from the devicetree //

//static const struct device *const dht11 = DEVICE_DT_GET(DT_ALIAS(my_sensor)); // Retrieve the Sensor device

//int main(void){
void main(void) {
    // Declare the varaibles required for the dht11 sensor
    //int ret;
    //struct sensor_value temp;
    //struct sensor_value hum;
    printk("=== STM32 Telemetry Simulator Booted ===\n");
    float base_temp = 22.5f;
    float base_hum = 45.0f;
    uint32_t timestamp = 0; 

    /* CHECK IF DEVICES ARE READY */

    //if(!device_is_ready(dht11)){
            //printk("Device %s is not ready.\n", dht11->name);
            //return 0;
        //}
    
    while(1){

        // Obtain the sensor readings
        //ret = sensor_sample_fetch(dht11);
        //if(ret < 0){
           //printk("Sample Fetch Error: %d\n", ret);
            //continue;
        //}

        //ret = sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        //if(ret < 0){
            //printk("Channel Get Error: %d\n", ret);
            //continue;
        //}

        //ret = sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &hum);
        //if(ret<0)
        //{
            //printk("Channel Get Error: %d\n", ret);
            //continue;
        //}
        timestamp = k_uptime_get_32();
        float temp = base_temp + (timestamp % 5) * 0.2f;
        float hum = base_hum - (timestamp % 3) * 0.5f;

        // Protocol format: TEMP:XX.XX,HUM:XX.XX,TS:XXXX\n
        //printk("TEMP:%d.%02d,HUM:%d.%02d,TS:%u\n", temp.val1, (temp.val2 / 10000), hum.val1, (hum.val2 / 10000), timestamp);
        printk("TEMP:%.2f,HUM:%.2f,TS:%u\n", temp, hum, timestamp);
        
        k_msleep(1000);
    }
    
    //return 0;
}
        
