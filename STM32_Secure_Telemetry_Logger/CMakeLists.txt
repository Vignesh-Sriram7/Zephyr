# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(STM32_Secure_Telemetry_Logger)

target_sources(app PRIVATE src/main.c)
