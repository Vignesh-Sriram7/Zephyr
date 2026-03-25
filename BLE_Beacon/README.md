# 🛰️ Secure Proximity Anchor  
High-Availability BLE Firmware with Recursive Signal Filtering

A deterministic, RTOS-based **Bluetooth Low Energy (BLE) peripheral** designed for industrial proximity sensing.  
Built on **Zephyr RTOS** for the **ESP32**, this project combines **real-time signal processing**, **hardware fail-safes**, and **secure wireless communication**.

---

# 🚀 Key Technical Features

## 1. Discrete Kalman Filter (Signal Integrity)

To mitigate the inherent noise and multi-path fading of **2.4 GHz RSSI signals**, a **recursive Kalman Filter** is implemented.

**Result**
- Raw RSSI values become smooth and predictable
- More stable distance estimation

**Concept**
- Process Noise → `Q`
- Measurement Noise → `R`
- Kalman Gain → `K`

The filter continuously updates the estimated distance using incoming RSSI measurements.

---

## 2. Deterministic Work Scheduling

Instead of using a CPU-intensive polling loop, the firmware relies on **Zephyr Global Work Queues**.

**RSSI Polling**
- Implemented with `k_work_delayable`
- Executes at **1 Hz**

**Benefits**
- Lower CPU usage
- Reduced power consumption
- Keeps BLE stack responsive

---

## 3. Hardware-Level Reliability (Watchdog)

Designed for **always-on industrial environments**.

**Supervision**
- Hardware Watchdog Timer (WDT)
- 10-second monitoring window

**Fail-Safe Behavior**
- The watchdog is refreshed inside the RSSI poller
- If the system deadlocks, the SoC performs a **cold reset**

---

## 4. Hardened Security & Persistence

**LESC (Secure Connections)**
- Passkey-based authenticated pairing
- Encrypted BLE communication

**NVS (Non-Volatile Storage)**
- Integrated with Zephyr Settings subsystem
- Stores bonding keys across power cycles

This enables **seamless reconnection** for trusted devices.

---

# 🛠️ Stack & Hardware

**RTOS**  
Zephyr RTOS

**SoC**  
ESP32 (Xtensa Dual-Core)

**Language**  
C (C11)

**Peripherals**
- BLE Radio
- Watchdog Timer (WDT)
- GPIO
- Flash Storage (NVS)

**Tools**
- west
- nRF Connect SDK
- ESP-IDF

---

# Project Structure
```
├── src/
│ └── main.c # Core BLE logic, Kalman filter, watchdog
│
├── boards/
│ └── esp32.overlay # Hardware mapping (WDT, GPIO aliases)
│
├── prj.conf # BLE, security, and NVS configuration
├── CMakeLists.txt
└── README.md
```
---

## Implementation Details

| Feature | Implementation |
|--------|---------------|
| Sampling Rate | 1 Hz (1000 ms interval) |
| Watchdog Timeout | 10 seconds |
| Distance Model | Log-distance path loss model |
| Authentication Level | Encrypted + authenticated pairing |
| Scheduling | Work queue (`k_work_delayable`) |

---

## Expected Behavior

- Device boots and starts BLE advertising  
- Authenticated device connects and pairs  
- RSSI values are sampled periodically  
- Kalman filter produces stable distance estimates  
- Watchdog ensures continuous operation  

---

## Future Improvements

- Deep-sleep optimization for ultra-low power operation  
- OTA firmware updates using MCUmgr  
- Multi-device tracking with multiple Kalman filter instances  
- Distance-based alert or trigger system  

---

## Purpose

This project focuses on **high-reliability BLE firmware design** using Zephyr RTOS and demonstrates:

- Real-time signal filtering  
- Deterministic scheduling  
- Secure BLE communication  
- Fault-tolerant embedded firmware design  
