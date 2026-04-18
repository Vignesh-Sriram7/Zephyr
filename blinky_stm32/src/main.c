#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Get LED0 definition from devicetree */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    printk("System Initialized. Starting Blinky + UART Test...\n");

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        printk("LED Toggle!\n");
        k_msleep(1000);
    }
}