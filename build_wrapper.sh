#!/bin/bash
cd /Users/bharat/esp32/BluetoothBeacon

# Set up ESP-IDF environment
export IDF_PATH=/Users/bharat/.espressif/v5.5.2/esp-idf
export IDF_TOOLS_PATH=/Users/bharat/.espressif

# Source the export script to set up all tools
. $IDF_PATH/export.sh >/dev/null 2>&1

# Run idf.py with provided arguments
idf.py "$@"
