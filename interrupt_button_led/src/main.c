#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define DEBOUNCE_DELAY_MS 50

// LED struct
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC(DT_ALIAS(led_0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC(DT_ALIAS(led_1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC(DT_ALIAS(led_2), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC(DT_ALIAS(led_3), gpios);

// Button struct
static const struct gpio_dt_spec btn = GPIO_DT_SPEC(DT_ALIAS(my_button), gpios);

//Struct to hold the callback function
static struct gpio_callback btn_cb;

// Struct for holding the workqueue
static struct k_work_delayable button_work;

int ret;
int b0=0, b1=0, b2=0, b3=0;

//Callback function
void button_isr(const struct device *dev,
                struct gpio_callback *cb,
                uint32_t pins)
{
   // Add work to the workqueue
    k_work_reschedule(&button_work, K_MSEC(DEBOUNCE_DELAY_MS));
}

void button_work_handler(struct k_work *work)
{
    int state;

    // Read the state of the button (after the debounce delay)
    state = gpio_pin_get_dt(&btn);
    if (state < 0) {
        printk("Error (%d): failed to read button pin\r\n", state);
    } else if (state) 
    
    {
        printk("changing the pattern of the leds!\r\n");
        b0^=1;
    ret = gpio_pin_set_dt(&led0, b0);
    
    if(!b0)
    {
        b1^=1;
        ret = gpio_pin_set_dt(&led1, b1);
        
	    }

        if(!b1)
        {
            b2^=1;
            ret = gpio_pin_set_dt(&led2, b2);
            
            if(!b2)
            {
                b3^=1;
                ret = gpio_pin_set_dt(&led3, b3);
                
            }
        }
    }
    //Re-enable the button interrupt after debounce
    gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);
}



int main (void)
{
    int ret;

    // Initialize work item
    k_work_init_delayable(&button_work, button_work_handler);

    // Checking if the leds are initialized
    if(!gpio_is_ready_dt(&led0)) {
        return 0;
    }

    if(!gpio_is_ready_dt(&led1)) {
        return 0;
    }

    if(!gpio_is_ready_dt(&led2)) {
        return 0;
    }

    if(!gpio_is_ready_dt(&led3)) {
        return 0;
    }

    //Check if the button is initialized
    if(!gpio_is_ready_dt(&btn)){
        return 0;
    }

    //Set all the GPIOS as output
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
        if (ret < 0) {
            return 0;
        }

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
        if (ret < 0) {
            return 0;
        }

    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
        if (ret < 0) {
            return 0;
        }

    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT);
        if (ret < 0) {
            return 0;
        }

    //Set the button as input
    ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);
    if (ret < 0) {
        return 0;
    }

    //Configure the interrupt to be tied to the button
    ret = gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        printk("ERROR: could not configure button as interrupt source\r\n");
        return 0;
    } 

    //connect the callback function to the interrupt source
    gpio_init_callback(&btn_cb, button_isr, BIT(btn.pin));
	gpio_add_callback(btn.port, &btn_cb);

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;

}