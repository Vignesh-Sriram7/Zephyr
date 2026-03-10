cmake_minimum_required(VERSION 3.20.0)

# 1. Point to the MCUboot source for the bootloader build (The "Child Image")
set(child_image_mcuboot_SOURCE_DIR "${CMAKE_SOURCE_DIR}/modules/bootloader/mcuboot/boot/zephyr")

# 2. DO NOT use ZEPHYR_EXTRA_MODULES here for the local folders
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OTA_BLE_SmartLock)

target_sources(app PRIVATE src/main.c)

# 3. Manually add the includes so your main.c compiles
zephyr_include_directories(
    ${CMAKE_SOURCE_DIR}/modules/bootloader/mcuboot/boot/bootutil/include
    ${CMAKE_SOURCE_DIR}/modules/bootloader/mcuboot/boot/zephyr/include
    ${CMAKE_SOURCE_DIR}/modules/zcbor/include
)