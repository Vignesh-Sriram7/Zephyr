cmake_minimum_required(VERSION 3.20.0)

# 1️⃣ Tell MCUboot where the Zephyr child image source is
set(child_image_mcuboot_SOURCE_DIR "${CMAKE_SOURCE_DIR}/modules/bootloader/mcuboot/boot/zephyr")

# 2️⃣ Tell MCUboot to use your custom mcuboot.conf
set(child_image_mcuboot_CONF_FILE "${child_image_mcuboot_SOURCE_DIR}/mcuboot.conf")

# 3️⃣ Force the unsupported ESP32 revision flag
set(mcuboot_CONFIG_ESP32_USE_UNSUPPORTED_REVISION y CACHE STRING "" FORCE)

# 4️⃣ Find Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OTA_BLE_SmartLock)

# 5️⃣ Add your app sources
target_sources(app PRIVATE src/main.c)