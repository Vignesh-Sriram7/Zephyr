#include <stdio.h>
#include <zephyr/kernel.h> // Contains the threading fucntions and mutex functions--> also timing macros
#include <zephyr/device.h> // Device onfigurations
#include <zephyr/drivers/sensor.h> // BME280 implements this API
#include <zephyr/drivers/display.h> // SSD1306 implements this API


// Define the sleep time used in the threads
static const int32_t sensor_sleep_ms = 500;
static const int32_t display_sleep_ms = 1000;

// Sleep time for the sensor capture
static const int32_t sleep_time_ms = 1000;

static uint8_t oled_buf[128];  // 1 page = 8px height

static int temp_int;   // shared temperature (integer °C)

/* A hard‑coded 8×8 pixel font for numbers
 Each digit (0–9) is drawn as an 8‑byte, 8‑pixel‑tall pattern.
Every byte represents one row of pixels, and each bit in that byte is on/off for the OLED*/

static const uint8_t digit_font[10][8] = {
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x3C,0x00}, // 1
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 2
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 3
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // 4
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 5
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 6
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // 7
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 8
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}  // 9
};

// Define the stack size of each thread
#define SENSOR_THREAD_STACK_SIZE 512
#define DISPLAY_THREAD_STACK_SIZE 512

// Define stack areas for both the threads
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(display_stack, DISPLAY_THREAD_STACK_SIZE);

// Declare thread data structs
static struct k_thread sensor_thread;
static struct k_thread display_thread;

// Define mutex
K_MUTEX_DEFINE(temp_mutex);

//Get device configurations
static const struct device *const bme280 = DEVICE_DT_GET(DT_ALIAS(my_temp));
static const struct device *const ssd1306 = DEVICE_DT_GET(DT_ALIAS(my_disp));

// Sensor thread start function 
void sensor_thread_start(void *arg_1, void *arg_2, void *arg_3)
{
    int ret;
    struct sensor_value temp;
    while(1){

        // Use the sensor sample fetch rather than the bme280 dedicated function
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
        // Lock the mutex variable which is the temp.val1 accessed by both the threads
        k_mutex_lock(&temp_mutex, K_FOREVER);
        temp_int = temp.val1;
        k_mutex_unlock(&temp_mutex);
        printk("Temperature: %d.%06d\n", temp.val1, temp.val2);
        k_msleep(sensor_sleep_ms);
    }
}

// Display thread start function
void display_thread_start(void *arg_1, void *arg_2, void *arg_3)
{
     while (1) {
        int local_temp;

        k_mutex_lock(&temp_mutex, K_FOREVER);
        local_temp = temp_int;
        k_mutex_unlock(&temp_mutex);

        memset(oled_buf, 0x00, sizeof(oled_buf)); // Clears the OLED display

        // Extract the digits from the temprature value
        int tens = (local_temp / 10) % 10; 
        int ones = local_temp % 10;

        // Copies the 8×8 bitmap for each digit into the display buffe
        memcpy(&oled_buf[0], digit_font[tens], 8);
        memcpy(&oled_buf[8], digit_font[ones], 8);

        // Describes the buffer to the SSD1306 driver
        struct display_buffer_descriptor desc = {
            .width = 128,
            .height = 8,
            .pitch = 128,
            .buf_size = sizeof(oled_buf),
        };

        display_write(ssd1306, 0, 0, &desc, oled_buf);

        k_msleep(display_sleep_ms);  // update every 0.5s
    }
}


int main(void)
{
    // Create thread ids
    k_tid_t sensor_tid;
    k_tid_t display_tid;

    if(!device_is_ready(bme280)){
        printk("Device %s is not ready.\n", bme280->name);
        return 0;
    }

    if(!device_is_ready(ssd1306)){
        printk("Device %s is not ready.\n", ssd1306->name);
        return 0;
    }
    // Start the sensor thread
    sensor_tid = k_thread_create(&sensor_thread,          // Thread struct
                                sensor_stack,            // Stack
                                K_THREAD_STACK_SIZEOF(sensor_stack),
                                sensor_thread_start,     // Entry point
                                NULL,                   // arg_1
                                NULL,                   // arg_2
                                NULL,                   // arg_3
                                7,                      // Priority
                                0,                      // Options
                                K_NO_WAIT);             // Delay

    // Start the display thread
    display_tid = k_thread_create(&display_thread,          // Thread struct
                                display_stack,            // Stack
                                K_THREAD_STACK_SIZEOF(display_stack),
                                display_thread_start,     // Entry point
                                NULL,                   // arg_1
                                NULL,                   // arg_2
                                NULL,                   // arg_3
                                8,                      // Priority
                                0,                      // Options
                                K_NO_WAIT);             // Delay


        
    while (1) {
        k_msleep(sleep_time_ms);
    }   
    return 0;
}
