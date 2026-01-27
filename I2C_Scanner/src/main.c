#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_ALIAS(my_i2cbus));

int main(void){
    int ret;
    uint8_t dummy = 0x00;

    if(!device_is_ready(i2c_dev)){
        printk("Device %s is not ready.\n", i2c_dev->name);
        return 0;
    }
    printk("I2C scanner started\n");
    printk("Scanning addresses 0x03 to 0x77\n");

    for(uint8_t addr = 0x03; addr <= 0x77; addr++){

        /* Zero-length write is enough to test ACK */
        ret = i2c_write(i2c_dev, &dummy, 0, addr);

        if (ret == 0) {
            printk("✔ Device found at 0x%02X\n", addr);
        }

        k_msleep(5);  // small delay to be polite on the bus
    }

    printk("I2C scan complete\n");

    return 0;
}