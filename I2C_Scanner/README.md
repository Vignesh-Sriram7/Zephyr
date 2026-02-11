# I2C Bus Scanner (Zephyr RTOS)

## Overview
This project implements a simple **I2C bus scanner** using **Zephyr RTOS**.  
It scans all valid I2C addresses (`0x03`–`0x77`) and prints any responding devices to the serial console.

Useful for:
- Verifying I2C wiring  
- Detecting connected sensors/peripherals  
- Debugging hardware communication  
- Learning Zephyr I2C basics  

The scanner performs a zero-length write to each address and checks for an ACK response.

---

## Hardware Used
- **Board:** Any Zephyr-supported board with I2C (ESP32 DevKitC, STM32, nRF52, etc.)  
- **I2C Peripheral:** Configurable I2C pins (SDA/SCL)  
- **Serial Console:** UART output for reading scan results  

Optional external hardware:
- Any I2C device (sensor, EEPROM, display, etc.)  
- Pull-up resistors on SDA and SCL (typically 4.7kΩ)

### Pin Configuration Example (ESP32)
| Signal | GPIO |
|--------|------|
| I2C SDA | GPIO15 |
| I2C SCL | GPIO16 |

---

## Software Stack
- **RTOS:** Zephyr RTOS (v3.x compatible)  
- **Drivers Used:**
  - `i2c`
- **Subsystems:**
  - Device tree aliases
  - Custom pinctrl configuration
  - printk logging

---

## Project Structure
```
├── boards/
│ └── your_board.overlay # Devicetree overlay with I2C config
├── src/
│ └── main.c # I2C scanner logic
├── prj.conf # Zephyr configuration
├── CMakeLists.txt
└── README.md
```
