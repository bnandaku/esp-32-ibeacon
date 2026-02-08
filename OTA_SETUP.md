# OTA (Over-The-Air) Update Setup Guide

This guide explains how to set up WiFi OTA updates for your ESP32 iBeacon fleet, allowing you to update all beacons remotely without physically connecting to them.

## 📋 Overview

With OTA updates enabled:
- **Remote Updates**: Update all beacons from one location
- **Automatic**: Beacons check for updates every 5 minutes
- **Zero Downtime**: Beacons continue broadcasting during updates
- **Fleet Management**: Update 5, 10, or 100 beacons simultaneously

## 🚀 Quick Start

### Step 1: Configure WiFi Settings

Run menuconfig to set your WiFi credentials:

```bash
cd /Users/bharat/esp32/BluetoothBeacon
. ~/esp/esp-idf/export.sh
idf.py menuconfig
```

Navigate to: **"iBeacon Transmitter Configuration"**

Set:
- **WiFi SSID**: Your WiFi network name (e.g., "MyHome")
- **WiFi Password**: Your WiFi password
- **Firmware Upgrade URL**: `http://YOUR_IP:8080/beacon_firmware.bin`

Replace `YOUR_IP` with your computer's local IP address (e.g., `http://192.168.1.100:8080/beacon_firmware.bin`)

Save and exit menuconfig (press `S` then `Q`)

### Step 2: Build and Flash Initial Firmware

Build the OTA-enabled firmware:

```bash
idf.py build
```

Flash all your beacons with this initial OTA-enabled firmware:

```bash
# Connect each beacon via USB and flash
idf.py -p /dev/cu.usbserial-* flash

# Repeat for all beacons (this is the LAST time you need USB!)
```

### Step 3: Start the OTA Server

Make the OTA server executable:

```bash
chmod +x ota_server.py
```

Create firmware directory and copy the initial firmware:

```bash
mkdir -p firmware
cp build/BluetoothBeacon.bin firmware/beacon_firmware.bin
```

Start the server:

```bash
python3 ota_server.py
```

You should see:

```
============================================================
  ESP32 iBeacon OTA Update Server
============================================================

✓ Server running on:
  - Local:   http://localhost:8080/beacon_firmware.bin
  - Network: http://192.168.1.100:8080/beacon_firmware.bin

✓ Configure your beacons with:
  - menuconfig -> WiFi SSID and Password
  - menuconfig -> Firmware URL: http://192.168.1.100:8080/beacon_firmware.bin

✓ All beacons will check for updates every 5 minutes
============================================================
```

**Keep this server running!** All beacons will connect to it for updates.

## 🔄 Updating All Beacons Remotely

Once the OTA system is set up, future updates are simple:

### 1. Make Your Changes

Edit your beacon configuration in `main/main.c`:

```c
#define BEACON_MINOR 13        // Change minor number
#define ADVERTISING_INTERVAL_MS 100  // Adjust interval
#define TRANSMIT_POWER 0       // Change power
```

### 2. Build New Firmware

```bash
idf.py build
```

### 3. Deploy to Server

```bash
cp build/BluetoothBeacon.bin firmware/beacon_firmware.bin
```

### 4. Wait for Automatic Update

That's it! All beacons will:
1. Detect the new firmware within 5 minutes
2. Download and install it automatically
3. Reboot with the new configuration
4. Resume broadcasting

Watch the OTA server logs to see beacons updating:

```
[2024-02-05 14:23:15] 192.168.1.101 - "GET /beacon_firmware.bin HTTP/1.1" 200 -
✓ Serving firmware update (523,456 bytes) to 192.168.1.101

[2024-02-05 14:23:42] 192.168.1.102 - "GET /beacon_firmware.bin HTTP/1.1" 200 -
✓ Serving firmware update (523,456 bytes) to 192.168.1.102

[2024-02-05 14:24:08] 192.168.1.103 - "GET /beacon_firmware.bin HTTP/1.1" 200 -
✓ Serving firmware update (523,456 bytes) to 192.168.1.103
```

## 🏠 Running OTA Server as Background Service

To keep the OTA server running permanently:

### Option 1: Using screen (macOS/Linux)

```bash
screen -S ota-server
cd /Users/bharat/esp32/BluetoothBeacon
python3 ota_server.py

# Press Ctrl+A then D to detach
# Reconnect later with: screen -r ota-server
```

### Option 2: Using nohup

```bash
cd /Users/bharat/esp32/BluetoothBeacon
nohup python3 ota_server.py > ota_server.log 2>&1 &

# View logs: tail -f ota_server.log
# Stop: killall python3 (or find the process ID)
```

### Option 3: Using launchd (macOS) - Best for permanent setup

Create `/Users/bharat/Library/LaunchAgents/com.beaconota.server.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.beaconota.server</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/bin/python3</string>
        <string>/Users/bharat/esp32/BluetoothBeacon/ota_server.py</string>
    </array>
    <key>WorkingDirectory</key>
    <string>/Users/bharat/esp32/BluetoothBeacon</string>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/Users/bharat/esp32/BluetoothBeacon/ota_server.log</string>
    <key>StandardErrorPath</key>
    <string>/Users/bharat/esp32/BluetoothBeacon/ota_server_error.log</string>
</dict>
</plist>
```

Load the service:

```bash
launchctl load ~/Library/LaunchAgents/com.beaconota.server.plist
```

The server will now start automatically at boot and restart if it crashes!

## 🔍 Monitoring Updates

### Check Server Logs

```bash
tail -f ota_server.log
```

### Check Beacon Status via Serial Monitor

```bash
idf.py -p /dev/cu.usbserial-* monitor
```

You'll see:

```
[OTA] Checking for firmware updates...
[OTA] Starting OTA update...
[OTA] Connected to server
[OTA] Writing to flash: 0%
[OTA] Writing to flash: 25%
[OTA] Writing to flash: 50%
[OTA] Writing to flash: 75%
[OTA] Writing to flash: 100%
[OTA] ✓ OTA update successful! Rebooting...
```

## ⚙️ Configuration Options

### Change Update Check Interval

Edit `main/main.c`:

```c
#define OTA_CHECK_INTERVAL_SEC 300  // Check every 5 minutes
// Change to 60 for 1 minute, 3600 for 1 hour, etc.
```

### Change Server Port

Edit `ota_server.py`:

```python
PORT = 8080  # Change to any available port
```

Remember to update the URL in menuconfig!

## 🔒 Security Notes

- **HTTP Only**: This uses unencrypted HTTP. For production, use HTTPS with proper certificates.
- **Local Network**: Server should be on same network as beacons for security.
- **Firewall**: Ensure port 8080 (or your chosen port) is open on your firewall.

## 🐛 Troubleshooting

### Beacons Not Updating

1. **Check WiFi Connection**:
   - Verify SSID and password in menuconfig
   - Check if beacon can connect: `idf.py monitor`

2. **Check Server URL**:
   - Ensure URL in menuconfig matches server IP
   - Test URL in browser: `http://YOUR_IP:8080/beacon_firmware.bin`

3. **Check Server is Running**:
   ```bash
   ps aux | grep ota_server.py
   ```

4. **Check Firewall**:
   - Allow incoming connections on port 8080
   - Test with: `curl http://YOUR_IP:8080/beacon_firmware.bin`

### Server Shows "Firmware Not Found"

```bash
# Ensure firmware file exists
ls -lh firmware/beacon_firmware.bin

# If missing, copy it
cp build/BluetoothBeacon.bin firmware/beacon_firmware.bin
```

### Beacon Stuck in Boot Loop After Update

- Bad firmware was flashed
- Re-flash via USB with known-good firmware:
  ```bash
  idf.py -p /dev/cu.usbserial-* flash
  ```

## 📝 Summary

**First Time Setup (Once):**
1. Configure WiFi in menuconfig
2. Flash beacons via USB (last time!)
3. Start OTA server

**Every Update After (Remote):**
1. Edit code
2. Build: `idf.py build`
3. Deploy: `cp build/BluetoothBeacon.bin firmware/beacon_firmware.bin`
4. Wait (beacons auto-update within 5 minutes)

That's it! No more USB cables needed. 🎉
