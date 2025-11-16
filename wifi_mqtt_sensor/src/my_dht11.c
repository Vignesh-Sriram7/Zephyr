#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "my_dht11.h"

LOG_MODULE_REGISTER(dht11_sensor, CONFIG_SENSOR_LOG_LEVEL);

static const struct device *const dht =DEVICE_DT_GET(DT_ALIAS(my_dht11));

int dht11_init(void)
{
    
    if (!device_is_ready(dht)) {
        LOG_ERR("Device not ready");
        return -ENODEV;
    }

    LOG_INF("DHT11 initialized successfully");
    return 0;
}

int dht11_read(int *temp, int *humidity)
{
    struct sensor_value t, h;
    int ret;

    ret = sensor_sample_fetch(dht);
    if (ret) {
        LOG_ERR("Sample fetch error: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(dht, SENSOR_CHAN_AMBIENT_TEMP, &t);
    if (ret) {
        LOG_ERR("Temperature read error: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(dht, SENSOR_CHAN_HUMIDITY, &h);
    if (ret) {
        LOG_ERR("Humidity read error: %d", ret);
        return ret;
    }
     *temp = sensor_value_to_int(&t);
    *humidity = sensor_value_to_int(&h);

    LOG_DBG("Read DHT11 -> Temp=%d C, Hum=%d %%", *temp, *humidity);

    return 0;
}