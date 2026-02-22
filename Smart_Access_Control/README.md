# BLE Smart Lock with Servo Motor (ESP32)

## Overview
This project demonstrates how to use **Zephyr RTOS** on an **ESP32 DevKitC WROOM** to:

- Control a servo motor via PWM to physically lock/unlock a mechanism  
- Implement a BLE (Bluetooth Low Energy) Server with custom GATT services  
- Handle secure pairing using a 6-digit passkey and bonding  
- Store bonding data in flash memory (NVS)  
- Execute commands based on authenticated BLE write requests  

---

## Hardware Used
- **Board:** ESP32 DevKitC WROOM  
- **Servo Motor:** SG90 or MG90S hobby servo  
- **Power:** Micro-USB (ESP32) + External 5V for Servo (recommended)  

### Pin Configuration

| Signal | GPIO |
|--------|------|
| Servo PWM Signal | GPIO13 (LEDC Channel 0) |
| Common Ground | GND |

---

## Software Stack
- **RTOS:** Zephyr RTOS (v3.7.0+)  

**Drivers Used**
- pwm (Servo control)  
- bluetooth (BLE stack)  
- flash / nvs (Persistent storage)  

**Subsystems**
- GATT: Custom service with read/write characteristics  
- SMP: Security Manager Protocol for encrypted pairing  
- Settings: For storing bonding keys in flash  

---

## Project Structure
```
.
├── boards/
│ └── esp32_wroom_devkitc.overlay # PWM & Pin control definitions
├── src/
│ └── main.c # BLE logic & Servo callbacks
├── prj.conf # BLE, stack sizes, and NVS configs
├── CMakeLists.txt
└── README.md
```

## Devicetree Overview

**Devices**
- Servo Motor on LEDC channel 0 mapped to GPIO13  

**Aliases**
- `motor-0` → Points to the PWM LEDC node  

---

## Application Design

**Initialization**
- Boots Bluetooth stack  
- Registers security callbacks  
- Moves servo to LOCKED position  

**Advertising**
- Advertises as device name: `Lock`  
- Exposes custom 128-bit service UUID 

**Command Execution**
- `OPEN` command → Servo moves to 1.0 ms pulse → Status: UNLOCKED  
- `CLOSE` command → Servo moves to 2.0 ms pulse → Status: LOCKED 

**Security & Pairing**
- BLE Secure Pairing (SMP) enabled  
- Bonding supported and stored in flash (NVS)  
- Pairing occurs when required by the client  
- No passkey is requested unless explicitly configured  

**Persistence**
- Successfully paired devices are bonded  
- Bonding data stored in flash (NVS)

## Expected Behavior

1. Device powers on → Servo moves to LOCKED  
2. Phone scans and connects via BLE  
3. If not previously paired, device performs secure pairing  
4. After pairing, user sends OPEN or CLOSE command  
5. Servo actuates accordingly  
6. On future connections, bonded device reconnects without re-pairing  

## Common Pitfalls & Fixes

### Disconnect (Reason 0x13) after "OPEN"

**Cause:**  
- Power spike from the servo motor  
- Blocking calls (e.g., `k_msleep`) inside BLE write callback  

**Fix:**  
- Use a separate 5V power supply for the servo  
- Ensure common ground between ESP32 and servo  
- Keep BLE callbacks lightweight (avoid long delays inside them)  
- Offload motor movement to a work queue or separate thread  


### Device Not Visible in Scan

**Cause:**  
- Advertising packet exceeds the 31-byte BLE limit  

**Fix:**  
- Move the device name or 128-bit UUID to the Scan Response (`sd[]`)  
- Reduce advertising data size  
- Verify `CONFIG_BT_DEVICE_NAME` length 

### Bluetooth Initialization Failed (Error -12)

**Cause:**  
- `ENOMEM` (Out of Memory)  
- The ESP32 ran out of heap or stack space while initializing the Bluetooth stack  

**Fix:**  
Increase memory allocations in `prj.conf`:

- Increase System Heap
`CONFIG_HEAP_MEM_POOL_SIZE=16384`

- Increase Stack Sizes
`CONFIG_MAIN_STACK_SIZE=4096`
`CONFIG_BT_RX_STACK_SIZE=2048`

- Shrink Bluetooth Buffers to save RAM
`CONFIG_BT_BUF_ACL_RX_SIZE=76`
`CONFIG_BT_BUF_ACL_TX_SIZE=76`
## Expected Output

### Serial Console
- Device boots and initializes BLE  
- Displays status messages (LOCKED / UNLOCKED)  
- Shows pairing/bonding status when a device connects  

<p align="center">
  <img src="your_serial_output_image.png" width="400">
</p>

---

### Mobile App (BLE Client)
- Device appears as **Lock**  
- Connect and send `OPEN` or `CLOSE` command  
- Lock state updates accordingly  

<p align="center">
  <img src="your_mobile_app_image.png" width="300">
</p>

---

### Physical Behavior
- On boot → Servo moves to **LOCKED** position  
- On `OPEN` command → Servo rotates to unlock  
- On `CLOSE` command → Servo rotates back to lock  

<p align="center">
  <img src="your_hardware_setup_image.png" width="300">
</p>
