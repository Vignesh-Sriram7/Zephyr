#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/time_units.h>


#define MAX_TICKS 10000 // Threshold to move on from waiting for the reply ping of the ultrasonic sensor


// Sleep settings 
static const int32_t wait_time_ms = 10;

volatile uint32_t start_time = 0;    // Timer start time    
volatile uint32_t stop_time = 0;     // Timer stop time
uint32_t duration = 0;      // Total duration from sending the singal to getting it back
uint32_t pulse_ns = 0;      // Duty Cycle Pulse
volatile uint32_t angle = 0;

// Get devicetree configurations 

static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(motor_0));
static const struct gpio_dt_spec trig = GPIO_DT_SPEC_GET(DT_ALIAS(hc_trig),gpios);
static const struct gpio_dt_spec echo = GPIO_DT_SPEC_GET(DT_ALIAS(hc_echo),gpios);

// Struct to hold the GPIO callback for the change of echo pin
static struct gpio_callback echo_cb_data;

// Create the isr to record the start and stop time of the pulse transmission and reception
void echo_isr(const struct device *dev,
                struct gpio_callback *cb,
                uint32_t pins)
    {
        if(gpio_pin_get_dt(&echo))
        {
            start_time = k_cycle_get_32();
        }
        else
        {
            stop_time = k_cycle_get_32();
            
        }

        
        
    }

// Handler to do the computation
void k_work_handler(struct k_work *work){
    // If stop_time is 0 or less than start_time, the ISR never finished
    // because no object was detected in the 50ms window.
    if (stop_time <= start_time || stop_time == 0) {
        printk("Angle: %d | Distance: Out of Range\n", angle);
        return; 
    }

    duration = stop_time - start_time;
    uint64_t duration_us = (uint64_t)duration * 1000000 / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
    
    uint32_t distance_cm = (duration_us * 34) / 2000;
    printk("Angle: %d | Distance: %u cm\n", angle, distance_cm);

    // CRITICAL: Reset stop_time so the NEXT loop doesn't reuse this old data
    stop_time = 0; 
}

static K_WORK_DEFINE (echo_work, k_work_handler);

int main(void)
{
    int ret;
    

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

    // Configure the interrupt
    ret = gpio_pin_interrupt_configure_dt(&echo, GPIO_INT_EDGE_BOTH);
    if (ret < 0) {
        printk("ERROR: could not configure echo as interrupt source\r\n");
        return 0;
    }

    // Set the trigger to 0 initially
    gpio_pin_set_dt(&trig, 0);

    // Connect callback function (ISR) to interrupt source
    gpio_init_callback(&echo_cb_data, echo_isr, BIT(echo.pin));
    gpio_add_callback(echo.port, &echo_cb_data);
    
    while(1)
    {
        for(angle = 0; angle<=180; angle+=5)
        {
            pulse_ns = 500000 + (angle * 2000000 / 180);

            pwm_set_pulse_dt(&servo, pulse_ns);
            
            // Fire the trigger pulse
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            k_msleep(50);

            k_work_submit(&echo_work);

        }

        for(angle = 180; angle>=0; angle-=5)
        {
            pulse_ns = 500000 + (angle * 2000000 / 180);
            pwm_set_pulse_dt(&servo, pulse_ns);
            // Fire the trigger pulse
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            k_msleep(50);

            k_work_submit(&echo_work);

        }
    }
}