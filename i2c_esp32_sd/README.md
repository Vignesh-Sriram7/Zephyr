# ESP32 I2C Temperature Display (BME280 + SSD1306)

This project demonstrates how to use **Zephyr** on an **ESP32 DevKitC WROOM** to:

* Read temperature data from a **BME280 sensor** over I2C
* Display the temperature on an **SSD1306 OLED (128x64)** display
* Use **multithreading** with a **shared temperature variable**
* Protect shared data using a **mutex**
* Configure devices cleanly using **Devicetree overlays**

---

## Hardware Used

* **Board**: ESP32 DevKitC WROOM
* **Temperature Sensor**: BME280 (I2C, address `0x77`)
* **Display**: SSD1306 OLED (I2C, address `0x3C`, 128x64)

### I2C Pin Configuration

| Signal | GPIO   |
| ------ | ------ |
| SDA    | GPIO15 |
| SCL    | GPIO16 |

---

## Software Stack

* **RTOS**: Zephyr RTOS (v3.7.x)
* **Drivers Used**:

  * `bosch,bme280`
  * `solomon,ssd1306fb`
* **Subsystems**:

  * I2C
  * Sensors
  * Display
  * Kernel threads
  * Mutex synchronization

---

## Project Structure

```
.
├── boards/
│   └── esp32_wroom_devkitc.overlay   # Devicetree overlay
├── src/
│   └── main.c                        # Application logic
├── prj.conf                          # Zephyr configuration
├── CMakeLists.txt
└── README.md
```

---

## Devicetree Overview

### Devices

* **BME280** on I2C0 at address `0x77`
* **SSD1306 OLED** on I2C0 at address `0x3C`

### Aliases

Aliases are defined to simplify device access in code:

```dts
aliases {
    my-temp = &my_bme280_i2c;
    my-disp = &my_ssd1306_i2c;
};
```

These are accessed in code using:

```c
DEVICE_DT_GET(DT_ALIAS(my_temp));
DEVICE_DT_GET(DT_ALIAS(my_disp));
```

---

## Application Design

### Threads

The application uses **two threads**:

1. **Sensor Thread**

   * Periodically reads temperature from the BME280
   * Stores the value in a shared variable

2. **Display Thread**

   * Reads the shared temperature value
   * Displays it on the SSD1306 OLED

---

### Mutex Usage

A **mutex** is used to protect the shared temperature variable:

* Prevents race conditions
* Ensures data consistency between threads

```c
k_mutex_lock(&temp_mutex, K_FOREVER);
// access shared temperature
k_mutex_unlock(&temp_mutex);
```

---

## Configuration (`prj.conf`)

```ini
# Core
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_MULTITHREADING=y

# I2C
CONFIG_I2C=y

# Sensors
CONFIG_SENSOR=y
CONFIG_BME280=y

# Display
CONFIG_DISPLAY=y
CONFIG_SSD1306=y

# Output
CONFIG_PRINTK=y
CONFIG_LOG=y
```

---

## Build & Flash

### Build

```sh
west build -b esp32_devkitc_wroom/esp32/procpu
```

### Flash

```sh
west flash
```

---

## Expected Output

* Serial console prints temperature values periodically
* OLED display shows the temperature in real time

Example:

```
Temperature: 25.43 C
```

---

## Common Pitfalls & Fixes

### Undefined reference to `__device_dts_ord_xx`

Cause: Device not instantiated in Devicetree

✔ Fix:

* Correct `compatible` string
* Ensure `status = "okay"`
* Use correct I2C address format (`@3c`, not `@3C`)

---

### `DT_HAS_SOLOMON_SSD1306FB_ENABLED = n`

Cause: SSD1306 node missing required properties

✔ Fix:

* Add `width` and `height`
* Use `compatible = "solomon,ssd1306fb"`

---

## Key Takeaways

* Zephyr device drivers are enabled **by Devicetree**, not manually
* `__device_dts_ord_xx` linker errors almost always mean **DT issues**
* Mutexes are essential when sharing data between threads
* Aliases greatly simplify device access

---
**Project status: Working & stable**


