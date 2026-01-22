# Zephyr ESP32 – 4‑Bit LED Binary Counter

This project demonstrates a **4‑bit binary counter** implemented on an **ESP32 DevKitC WROOM** using **Zephyr RTOS**. Four GPIO‑connected LEDs represent a binary count that increments every second.

---

## Project Overview

* Platform: **ESP32 DevKitC WROOM**
* RTOS: **Zephyr OS**
* Language: **C**
* Peripherals: **GPIO (LEDs)**
* Pattern: **4‑bit binary counter**

Each LED corresponds to one bit:

|  LED |   GPIO | Bit Weight |
| ---: | -----: | ---------: |
| LED0 | GPIO16 |   2⁰ (LSB) |
| LED1 | GPIO17 |         2¹ |
| LED2 | GPIO18 |         2² |
| LED3 | GPIO19 |   2³ (MSB) |

The counter increments every **1000 ms** and rolls over naturally after `1111`.

---

## Project Structure

```
.
├── src/
│   └── main.c            # Application source code
├── boards/
│   └── <board>.overlay   # Devicetree overlay for LEDs
├── prj.conf              # Zephyr configuration
└── README.md             # Project documentation
```

---

## Devicetree Overlay

The devicetree overlay defines four LEDs and exposes them using **aliases**, allowing the application to remain hardware‑agnostic.

### LED Aliases

```dts
/ {
    aliases {
        led-0 = &myled0;
        led-1 = &myled1;
        led-2 = &myled2;
        led-3 = &myled3;
    };
};
```

### GPIO LED Definitions

```dts
leds {
    compatible = "gpio-leds";

    myled0: d16 {
        gpios = <&gpio0 16 GPIO_ACTIVE_HIGH>;
    };

    myled1: d17 {
        gpios = <&gpio0 17 GPIO_ACTIVE_HIGH>;
    };

    myled2: d18 {
        gpios = <&gpio0 18 GPIO_ACTIVE_HIGH>;
    };

    myled3: d19 {
        gpios = <&gpio0 19 GPIO_ACTIVE_HIGH>;
    };
};
```

These aliases are accessed in code using `DT_ALIAS(led_0)` through `DT_ALIAS(led_3)`.

---

## Application Logic

### Key Features

* Uses **Zephyr GPIO devicetree API** (`gpio_dt_spec`)
* Verifies GPIO readiness before use
* Configures all LEDs as outputs
* Implements binary counting using bit‑toggle logic

### Binary Counter Logic

* `b0` toggles every cycle
* When `b0` overflows, `b1` toggles
* This cascades up to `b3`

This mimics how a real binary counter works in hardware.

---

## Timing

The counter updates every **1 second**:

```c
static const int32_t sleep_time = 1000;
...
k_msleep(sleep_time);
```

---

## Required Zephyr Configuration (`prj.conf`)

```conf
CONFIG_GPIO=y
CONFIG_PRINTK=y
CONFIG_LOG=y
```

---

## Build & Flash

```bash
west build -b esp32_devkitc_wroom/esp32/procpu
west flash
```

---

## Expected Output

The LEDs count in binary:

```
0000
0001
0010
0011
0100
...
1111
```

Each LED turning ON represents a binary `1`.

---

## Notes

* GPIO pins can be changed easily by editing the devicetree overlay
* The application is fully portable to other boards supporting GPIO
* No polling or busy‑waits — Zephyr scheduling is used

