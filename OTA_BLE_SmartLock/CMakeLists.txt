cmake_minimum_required(VERSION 3.20.0)


set(child_image_mcuboot_SOURCE_DIR "${CMAKE_SOURCE_DIR}/modules/bootloader/mcuboot/boot/zephyr")

set(child_image_mcuboot_CONF_FILE "${child_image_mcuboot_SOURCE_DIR}/mcuboot.conf")

set(mcuboot_CONFIG_ESP32_USE_UNSUPPORTED_REVISION y CACHE STRING "" FORCE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OTA_BLE_SmartLock)


target_sources(app PRIVATE src/main.c)