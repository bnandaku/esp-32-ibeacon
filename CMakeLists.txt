# ESP32 iBeacon Transmitter
# Professional firmware for ESP32-based iBeacon broadcasting
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Use custom partition table for OTA support
set(PARTITION_CSV_PATH "${CMAKE_SOURCE_DIR}/partitions_ota.csv")

# Optimized build - only include essential components
idf_build_set_property(MINIMAL_BUILD ON)

project(esp32-ibeacon-transmitter VERSION 3.2.5)
