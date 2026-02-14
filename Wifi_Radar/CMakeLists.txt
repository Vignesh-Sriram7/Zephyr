cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Wifi_Radar)

FILE(GLOB app_sources src/*.c) # Make a list of every file in src ending with .c and store it in app_sources
target_sources(app PRIVATE ${app_sources}) # Ask the coomplier to build evryfile in the list app_sources