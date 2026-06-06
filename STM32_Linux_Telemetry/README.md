# Linux_Telemetry_Pipeline
Cross-Boundary Embedded Telemetry Streaming from STM32 to Linux (WSL2)

A deterministic embedded telemetry pipeline that bridges a **Zephyr RTOS-based STM32 Nucleo board** with a native **Ubuntu (WSL2) Linux environment**. The system generates environmental telemetry data, transports it over **USB CDC Virtual COM**, and processes it using a custom **C++ POSIX-compliant parser engine**.

---

# Key Technical Features

## USB CDC Telemetry Transport

The STM32 firmware exposes itself as a standard USB Communication Device Class (CDC) endpoint.

**Benefits**
- No custom drivers required
- Enumerates as a standard serial device
- Cross-platform compatibility

**Linux Interface**
```bash
/dev/ttyACM0
```

## System Architecture

```text
┌──────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│   Produce Data   │     │  Transport Data  │     │   Consume Data   │
└────────┬─────────┘     └────────┬─────────┘     └────────┬─────────┘
         │                        │                        │
         ▼                        ▼                        ▼

  STM32 Nucleo            USB CDC Interface         Ubuntu (WSL2)
 (Zephyr RTOS)          (/dev/ttyACM0 VCP)        (C++ Parser Engine)

Simulated/BME280         Virtual COM Port           POSIX Streams
     Sensor                    USB                        Host

         └──────────────────────►──────────────────────────►
                   Structured Telemetry Packets
```

---

## Structured Telemetry Generation

The firmware continuously generates environmental telemetry packets.

**Metrics**
- Temperature
- Humidity
- Timestamp

**Generation Method**
- Time-based modulo tracking algorithm
- Periodic telemetry scheduling
- Structured string packet formatting

This provides deterministic sensor simulation while maintaining low firmware complexity.

---

## POSIX-Based Serial Processing

The host-side telemetry engine follows the Linux philosophy:

> Everything is a file

The serial interface is consumed directly through standard file streams.

**Implementation**
```cpp
std::ifstream serial_port("/dev/ttyACM0", std::ios::in | std::ios::binary);;
```

**Benefits**
- Native Linux integration
- No third-party serial libraries
- Portable POSIX design

---

## Optimized Stream Handling

The parser differentiates between normal telemetry output and critical failures.

### Buffered Output

```cpp
std::cout
```

Used for:
- Parsed telemetry
- Status messages
- Routine diagnostics

### Unbuffered Output

```cpp
std::cerr
```

Used for:
- Parsing failures
- Device communication errors
- Runtime exceptions

This separation improves responsiveness while maintaining efficient CPU utilization.

---

## Floating-Point Telemetry Support

Zephyr's default minimal C library omits floating-point formatting.

To enable environmental telemetry values:

```ini
CONFIG_CBPRINTF_FP_SUPPORT=y
CONFIG_MINIMAL_LIBC=n
CONFIG_PICOLIBC=y
```

**Result**
- Native float printing support
- Accurate temperature reporting
- Accurate humidity reporting

Without these options:

```c
printk("Temp: %f", temp);
```

May output:

```text
Temp: *float*
```

---

# Stack & Hardware

**RTOS**  
Zephyr RTOS

**Target MCU**  
STM32 Nucleo Development Board

**Language**  
C (Firmware)

**Host Application**  
C++

**Communication**
- USB CDC
- Virtual COM Port

**Operating Systems**
- Windows 11
- Ubuntu (WSL2)

**Tools**
- Zephyr SDK
- west
- usbipd-win
- GCC / G++

---

# Project Structure

```text
├── firmware/
│   ├── src/
│   │   └── main.c
│   │
│   ├── boards/
│   │   └── nucleo_f303re.overlay
│   │
│   ├── prj.conf
│   └── CMakeLists.txt
│
├── linux/
│   └── serial_reader.cpp
│
└── README.md
```

---

## Implementation Details

| Feature | Implementation |
|----------|---------------|
| RTOS | Zephyr RTOS |
| Communication | USB CDC |
| Host Interface | `/dev/ttyACM0` |
| Data Format | Structured Text Packets |
| Host Parser | POSIX C++ |
| Float Support | Picolibc + CBPRINTF |
| Development Environment | WSL2 Ubuntu |
| Device Bridge | USB/IP (usbipd-win) |

---

## Deployment Workflow

### Attach Device to WSL2

```powershell
usbipd list
usbipd bind --busid <BUS_ID>
usbipd attach --wsl --busid <BUS_ID>
```

### Configure Serial Interface

```bash
sudo stty -F /dev/ttyACM0 115200 raw -clocal -echo
```

### Verify Incoming Telemetry

```bash
cat /dev/ttyACM0
```

### Compile Host Parser

```bash
g++ serial_reader.cpp -o telemetry_engine
```

### Run Telemetry Engine

```bash
sudo ./telemetry_engine
```
Note: Run the process to attach the device to WSL on Windows powershell as administrator
---

## Expected Behavior

- STM32 boots and initializes Zephyr RTOS
- Environmental telemetry is generated periodically
- USB CDC enumerates as `/dev/ttyACM0`
- Device is attached to WSL2 using USB/IP
- Linux receives telemetry packets
- C++ parser extracts structured data
- Telemetry is displayed and validated in real time

---

## Devicetree Configuration Example

For GPIO-based sensors such as DHT11:

```dts
dht11 {
    dio-gpios = <&gpioa 0 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
};
```

Proper GPIO allocation ensures successful sensor initialization and driver binding.

---

## Purpose

This project focuses on **embedded-to-Linux telemetry integration** and demonstrates:

- Zephyr RTOS firmware development
- USB CDC communication
- WSL2 hardware passthrough
- POSIX-compliant serial processing
- Embedded telemetry streaming
- Cross-platform hardware/software integration

## Output

<p align="center">
  <img src="Screenshot 2026-06-06 142554.png" width="450">
</p>
