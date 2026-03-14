# Zephyr Embedded Projects

This repository contains a collection of **embedded systems projects built using Zephyr RTOS** across multiple microcontroller platforms such as **ESP32 and STM32**.  
The projects demonstrate concepts including **Bluetooth Low Energy (BLE), WiFi, MQTT communication, device drivers, interrupts, PWM control, and sensor interfacing**.

The goal of this repository is to explore **real-world embedded systems design using Zephyr's device model, Devicetree configuration, and RTOS features**.

---

## Platforms Used

- **ESP32 DevKitC WROOM**
- **STM32F303RE**
- Standard embedded peripherals (GPIO, PWM, I2C, WiFi, BLE)

---

## Technologies & Concepts

- Zephyr RTOS
- Bluetooth Low Energy (BLE)
- WiFi Networking
- MQTT Communication
- Device Drivers (GPIO, PWM, I2C)
- Devicetree & Pin Control
- Interrupt Handling
- Work Queues & Multithreading
- Sensor Integration
- Embedded Web Interfaces

---

# Projects

## BLE

### BLE Find Me
Simple BLE broadcaster implementing a **Find-Me style beacon** that advertises presence to nearby devices.

### OTA BLE Smart Lock
A **Bluetooth-controlled smart lock** using an ESP32 and a servo motor.  
Supports authenticated BLE commands and persistent device bonding.

### Smart Access Control
A BLE-based access system that allows **authorized devices to trigger lock/unlock actions** using custom GATT services.

---

## Sensors & Embedded Interfaces

### I2C Scanner
Utility tool that scans the **I2C bus and detects connected devices**. Useful for debugging sensors and displays.

### I2C Temperature Sensor + OLED
Reads temperature data from a **BME280 sensor** and displays it on an **SSD1306 OLED display** using I2C.

### WiFi MQTT Sensor Node
ESP32 sensor node that reads environmental data and **publishes it to an MQTT broker over WiFi**.

---

## Robotics & Motion Projects

### Ultrasonic Radar
A **servo-driven radar scanner** using the HC-SR04 ultrasonic sensor to measure distances while sweeping angles.

### Ultrasonic Radar (Interrupt Version)
Improved radar implementation using **GPIO interrupts** for precise echo timing instead of polling.

### WiFi Radar
Extends the radar project by **streaming distance data to a web interface** where the scan results are visualized in real time.

---

## Embedded Fundamentals

### Button Interrupt LED
Demonstrates **GPIO interrupts** by triggering LED patterns when a button is pressed.

### LED Binary Pattern Generator
Displays **binary counting patterns using LEDs**, illustrating GPIO control and timing.

---

## Board Support & Experiments

### STM32F303RE Custom Board Setup
Initial setup and configuration for running **Zephyr on a custom STM32F303RE board**.

---

# Repository Structure


.
├── BLE_Find_Me
├── OTA_BLE_SmartLock
├── Smart_Access_Control
├── I2C_Scanner
├── I2C_TempSensor_OLED
├── Ultrasonic_Radar
├── Ultrasonic_Radar_Interrupt
├── Wifi_Radar
├── wifi_mqtt_sensor
├── interrupt_button_led
├── led_pattern_binary
└── stmf303r


Each folder contains an **independent Zephyr project** with its own:

- `src/`
- `prj.conf`
- `boards/*.overlay`
- `CMakeLists.txt`
- `README.md`

---
