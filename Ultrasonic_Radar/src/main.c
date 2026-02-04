#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/time_units.h>

#define MAX_TICKS 10000 // Threshold to move on from waiting for the reply ping of the ultrasonic sensor


// Sleep settings 
static const int32_t wait_time_ms = 10;

// Get devicetree configurations 

static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(motor_0));
static const struct gpio_dt_spec trig = GPIO_DT_SPEC_GET(DT_ALIAS(hc_trig),gpios);
static const struct gpio_dt_spec echo = GPIO_DT_SPEC_GET(DT_ALIAS(hc_echo),gpios);

int main()
{
    int ret;
    uint32_t counter=0;     // Counter for the start ping
    uint32_t counter2=0;    // Counter for the end ping
    uint32_t start_time = 0;    // Timer start time    
    uint32_t stop_time = 0;     // Timer stop time
    uint32_t duration = 0;      // Total duration from sending the singal to getting it back
    uint32_t pulse_ns = 0;      // Duty Cycle Pulse

    // Check if the trigger is ready
    if(!gpio_is_ready_dt(&trig))
        return 0;
    // Check if the echo is ready
    if(!gpio_is_ready_dt(&echo))
        return 0;
    // Check if the servo is ready
    if(!pwm_is_ready_dt(&servo))
        return 0;

    // Configure the trigger to output
    ret = gpio_pin_configure_dt(&trig, GPIO_OUTPUT);
    if(ret<0)
        return 0;

    // Configure the echo to output
    ret = gpio_pin_configure_dt(&echo, GPIO_INPUT);
    if(ret<0)
        return 0;

    // Set the trigger to 0 initially
    gpio_pin_set_dt(&trig, 0);

    while(1){

        // Loop over for the clock wise rotation of the servo motor
        for(int angle = 0; angle<=180; angle+=5)
        {
            pulse_ns = 500000 + (angle * 2000000 / 180);

            pwm_set_pulse_dt(&servo, pulse_ns);
            k_msleep(50);

            // Fire the trigger pulse
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            counter = 0;
            while(gpio_pin_get_dt(&echo) == 0){
                counter++;
                if(counter>MAX_TICKS)
                    break;
            }
            start_time = k_cycle_get_32();  

            counter2 = 0;
            while(gpio_pin_get_dt(&echo) == 1){
                counter2++;
                if(counter2>MAX_TICKS)
                    break;
            }
            stop_time = k_cycle_get_32();
            
            if(counter < MAX_TICKS && counter2 < MAX_TICKS)
            {
                duration = stop_time - start_time;
                uint64_t duration_us = (uint64_t)duration * 1000000 / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
                uint32_t distance_cm = (duration_us * 34) / 2000;
                printk("Angle: %d, Distance: %u cm\n", angle, distance_cm);
            }
            else
                printk("No object detected");
        }
        
        for(int angle = 180; angle>=0; angle-=5)
        {
            pulse_ns = 500000 + (angle * 2000000 / 180);

            pwm_set_pulse_dt(&servo, pulse_ns);
            k_msleep(50);
            // HC-SR04 code
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            counter = 0;
            while(gpio_pin_get_dt(&echo) == 0){
                counter++;
                if(counter>MAX_TICKS)
                    break;
            }
            start_time = k_cycle_get_32();  

            counter2 = 0;
            while(gpio_pin_get_dt(&echo) == 1){
                counter2++;
                if(counter2>MAX_TICKS)
                    break;
            }
            stop_time = k_cycle_get_32();
            
            if(counter < MAX_TICKS && counter2 < MAX_TICKS)
            {
                duration = stop_time - start_time;
                uint64_t duration_us = (uint64_t)duration * 1000000 / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
                uint32_t distance_cm = (duration_us * 34) / 2000;
                printk("Angle: %d, Distance: %u cm\n", angle, distance_cm);
            }
            else
                printk("No object detected");
        }
            
    k_msleep(wait_time_ms);    
    }
    
}