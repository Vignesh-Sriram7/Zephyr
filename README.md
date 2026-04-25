# Zephyr Embedded Projects

A collection of embedded systems projects built using the Zephyr RTOS on ESP32 and STM32 platforms. This repository demonstrates practical implementations of Bluetooth Low Energy (BLE), WiFi, MQTT, sensor interfacing, and real-time control systems.

---

## Overview
The purpose of this repository is to:
- Demonstrate hands-on experience with Zephyr RTOS
- Provide modular and reusable embedded system examples
- Showcase real-world IoT and control applications
- Integrate CI/CD practices for firmware development and validation

## Projects

### BLE
- BLE Beacon  
- BLE Find Me  
- OTA BLE Smart Lock  
- Smart Access Control  

### Sensors & Communication
- I2C Scanner  
- I2C Temp + OLED (BME280 + SSD1306)  
- WiFi MQTT Sensor Node  

### Robotics & Control
- Ultrasonic Radar  
- Ultrasonic Radar (Interrupt)  
- WiFi Radar  

### Fundamentals
- Button Interrupt LED  
- LED Binary Pattern  

### Board Support
- STM32F303RE Setup  

---

## Structure
```
├── BLE_Beacon
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
```

Each folder is a standalone Zephyr project with its own configuration and README.

---
## CI/CD Pipeline
This repository includes a continuous integration and deployment pipeline designed to ensure code quality and build reliability.

The pipeline performs the following steps:

- Builds all projects against supported targets
- Runs static checks and validation
- Verifies configuration consistency
- Generates firmware binaries

The pipeline is triggered on every push and pull request.

## Build Artifacts
Each successful pipeline run produces artifacts that can be downloaded and used for deployment or testing:

- Compiled firmware files (.bin)
- Build logs for debugging and traceability
- Per-project build outputs
