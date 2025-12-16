#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
static const int32_t sleep_time = 1000;

//Declare the defined leds from the device tree 
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC(DT_ALIAS(led_0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC(DT_ALIAS(led_1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC(DT_ALIAS(led_2), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC(DT_ALIAS(led_3), gpios);

int main (void)
{
    int ret;
    int state = 0;
    int b0=0, b1=0, b2=0, b3=0;

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

// Run the code forever
while (1)
{
    //logic for 4 bit binary counter
    b0^=1;
    ret = gpio_pin_set_dt(&led0, b0);
    if (ret < 0) {
		return 0;
	}
    if(!b0)
    {
        b1^=1;
        ret = gpio_pin_set_dt(&led1, b1);
        if (ret < 0) {
		return 0;
	    }

        if(!b1)
        {
            b2^=1;
            ret = gpio_pin_set_dt(&led2, b2);
            if (ret < 0) {
                return 0;
            }
            if(!b2)
            {
                b3^=1;
                ret = gpio_pin_set_dt(&led3, b3);
                if (ret < 0) {
                    return 0;
                }
                
            }
        }
    }
    k_msleep(sleep_time);
    
}
return 0;
}