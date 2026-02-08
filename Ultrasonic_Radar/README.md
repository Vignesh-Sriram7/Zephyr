# Ultrasonic Radar with servo motor (HC-SR04)

This project demonstrates how to use **Zephyr** on an **ESP32 DevKitC WROOM** to:

* Sweep a **servo motor** using PWM
* Measure distance using an **HC-SR04 ultrasonic sensor**
* Print **angle and distance readings** over the serial console
* Configure devices cleanly using **Devicetree overlays**

---

## Hardware Used

* **Board**: ESP32 DevKitC WROOM
* **Servo Motor**: Standard hobby servo (PWM controlled)
* **Ultrasonic Sensor**: HC-SR04

### Pin Configuration

| Signal          | GPIO   |
| --------------- | ------ |
| Servo Signal    | GPIO13 |
| HC-SR04 Trigger | GPIO16 |
| HC-SR04 Echo    | GPIO17 |

---
## Software Stack

* **RTOS**: Zephyr RTOS (v3.x compatible)
* **Drivers Used**:
  * `pwm`
  * `gpio`
* **Subsystems**:
  * PWM (Servo control)
  * GPIO (Trigger/Echo)
  * Timing & delays
  * printk logging

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

* **Servo Motor** on LEDC channel 0, GPIO13
* **HC-SR04 Trigger** on GPIO16
* **HC-SR04 Echo** on GPIO17

### Aliases

```dts
aliases {
    hc-trig = &hc_trigger;
    hc-echo = &hc_echo;
    motor-0 = &motor_0;
};
```
## Application Design

* Sweeps servo clockwise (0° → 180°) and counterclockwise (180° → 0°)

* At each angle:

  * Sets PWM pulse for servo

  * Fires HC-SR04 trigger pulse

  * Measures echo duration

  * Calculates distance in cm

  * Prints Angle and Distance to serial

## Configuration (`prj.conf`)

```ini
CONFIG_PWM=y
CONFIG_GPIO=y
CONFIG_PRINTK=y
CONFIG_PINCTRL=y
CONFIG_ESP32_USE_UNSUPPORTED_REVISION=y
```

## Expected Output
* Serial console prints angle and distance periodically

Example:
```
Angle: 0, Distance: 25 cm
Angle: 5, Distance: 24 cm
Angle: 10, Distance: 22 cm
```
## Common Pitfalls & Fixes
### Servo not moving

* Check PWM channel and pin configuration

* Verify `pwm_is_ready_dt(&servo)`

###HC-SR04 always reads "No object detected"

* Ensure Trigger is output, Echo is input

* Confirm wiring and GPIO pins match devicetree overlay
