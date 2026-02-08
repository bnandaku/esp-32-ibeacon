#!/bin/bash
set -e

echo "🔨 Building ESP32 Beacon Firmware..."
echo "Project: /project"

cd /project

# Source IDF environment
. $IDF_PATH/export.sh

# Clean build cache if it exists (to avoid path conflicts)
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build cache..."
    idf.py fullclean
fi

# Build the project
idf.py build

# Copy firmware to output directory
echo "📦 Copying firmware to /firmware..."
cp build/esp32-ibeacon-transmitter.bin /firmware/beacon_firmware.bin

echo "✅ Build complete!"
ls -lh /firmware/beacon_firmware.bin
