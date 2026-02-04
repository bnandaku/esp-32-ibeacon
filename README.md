# ESP32 iBeacon Transmitter

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v4.4+-blue.svg)](https://github.com/espressif/esp-idf)

Professional firmware to transform your ESP32 into a fully-featured iBeacon transmitter. Perfect for room detection, indoor positioning, presence awareness, and home automation.

## ✨ Features

- 🎯 **Simple Configuration** - Copy-paste UUID strings, no hex conversion needed
- ⚡ **Low Power** - Optimized for ~20-30mA consumption
- 🔧 **Highly Configurable** - Adjust power, interval, and identification
- 📱 **Universal Compatibility** - Works with iOS, Android, and all iBeacon apps
- 🏠 **Home Automation Ready** - Ideal for room detection and presence-based automation
- 📦 **Production Ready** - Clean, documented, professional code

## 📦 What You Need

### Hardware
- **ESP32 Development Board** (any model: ESP32, ESP32-C3, ESP32-S3, etc.)
- **USB Cable** (for programming and power)
- **Power Source** (USB power adapter, power bank, or battery)

### Software
- **ESP-IDF** (Espressif IoT Development Framework) v4.4 or newer
- **USB to Serial Driver** (usually automatic on macOS/Linux)

## 🚀 Quick Start Guide

### Step 1: Install ESP-IDF

#### macOS/Linux

```bash
# Install dependencies
# macOS:
brew install cmake ninja dfu-util

# Linux (Debian/Ubuntu):
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Download ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install ESP-IDF (this takes a few minutes)
./install.sh esp32

# Set up environment (add this to your ~/.zshrc or ~/.bashrc)
. ~/esp/esp-idf/export.sh
```

#### Windows

1. Download and run the [ESP-IDF Windows Installer](https://dl.espressif.com/dl/esp-idf/)
2. Follow the installation wizard
3. Open "ESP-IDF PowerShell" from Start Menu

### Step 2: Configure Your Beacon

Edit `main/main.c` and set your beacon parameters:

```c
// Your Beacon UUID - Just paste the UUID string here!
// Must match the UUID in your iOS app
#define BEACON_UUID_STRING "B9407F30-F5F8-466E-AFF9-25556B57FE6D"

// Major number - Group beacons (e.g., all beacons in your home)
#define BEACON_MAJOR 100

// Minor number - Unique ID for each room
// Living Room = 1, Bedroom = 2, Kitchen = 3, etc.
#define BEACON_MINOR 1

// How often to broadcast (milliseconds)
#define ADVERTISING_INTERVAL_MS 500

// Signal strength (-127 to 0 dBm)
// -4 = good for medium rooms (5-10m)
// -20 = small rooms (2-5m)
#define TRANSMIT_POWER -4
```

### Step 3: Build and Flash

```bash
# Navigate to project directory
cd BluetoothBeacon

# Set ESP-IDF environment (if not already set)
. ~/esp/esp-idf/export.sh

# Configure project for your ESP32 board
idf.py set-target esp32    # For ESP32
# OR
idf.py set-target esp32c3  # For ESP32-C3
# OR
idf.py set-target esp32s3  # For ESP32-S3

# Build the firmware
idf.py build

# Connect your ESP32 via USB and flash
# Replace /dev/cu.usbserial-XXXX with your port
idf.py -p /dev/cu.usbserial-* flash

# View serial output (optional, to see beacon status)
idf.py -p /dev/cu.usbserial-* monitor
```

**To exit monitor:** Press `Ctrl+]`

### Step 4: Verify It Works

After flashing, you should see:
```
========================================
  ESP32 iBeacon Transmitter
========================================
Initializing Bluetooth...
✓ Bluetooth initialized
Configuring iBeacon...
========== iBeacon Configuration ==========
UUID: B9407F30-F5F8-466E-AFF9-25556B57FE6D
Major: 100 (0x0064)
Minor: 1 (0x0001)
Measured Power: -59 dBm
==========================================
✓ iBeacon configured successfully
✓ iBeacon is broadcasting!
✓ Your device is now discoverable by iOS Room Detector
========================================
Setup complete! Beacon is now running.
Power consumption: ~20-30mA (optimized)
========================================
```

## 🏠 Setting Up Multiple Beacons

For room detection, you need one beacon per room. Here's how to configure multiple beacons:

### Option 1: Same UUID, Different Minor Numbers (Recommended)

**All beacons:**
- Same `BEACON_UUID`
- Same `BEACON_MAJOR`
- Different `BEACON_MINOR` for each room

```c
// Living Room beacon
#define BEACON_MINOR 1

// Bedroom beacon
#define BEACON_MINOR 2

// Kitchen beacon
#define BEACON_MINOR 3
```

### Option 2: Different UUIDs

Use different UUIDs for each beacon if you want complete separation.

## 🔧 Configuration Options Explained

### Beacon UUID
- **What it is:** A unique identifier for your beacon network
- **Should I change it?** Use a standard manufacturer UUID (Estimote, Kontakt.io) or generate your own
- **Where to get one:** https://www.uuidgenerator.net/
- **Format:** 32 hex characters with dashes: `XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX`

### Major Number (0-65535)
- **What it is:** Groups beacons together
- **Example:** All beacons in your home = Major 100, all beacons at office = Major 200
- **Typical use:** Keep the same for all beacons in one location

### Minor Number (0-65535)
- **What it is:** Unique identifier for individual beacons
- **Example:** Living Room = 1, Bedroom = 2, Kitchen = 3
- **Important:** Each beacon in a room needs a unique minor number

### Advertising Interval
- **What it is:** How often the beacon broadcasts
- **Range:** 100-10000 milliseconds
- **Recommendations:**
  - 100-300ms = Very responsive, higher battery usage
  - 500-1000ms = Good balance (recommended)
  - 2000-5000ms = Battery saver mode, slower detection

### Transmit Power
- **What it is:** Signal strength/range
- **Range:** -127 to 0 dBm
- **Recommendations:**
  - 0 dBm = Maximum range (~30m), highest battery usage
  - -4 dBm = Medium range (~10m), balanced (recommended)
  - -20 dBm = Short range (~5m), lower battery, reduces false detections
  - -40 dBm = Very short range (~2m), minimal battery

## 📍 Beacon Placement Tips

1. **Central location:** Place beacon in the center of the room, not in corners
2. **Height:** Wall-mounted at 1.5-2m height works best
3. **Avoid obstacles:** Keep away from large metal objects
4. **Power:** Use USB power adapter for permanent installation
5. **Battery:** Use power bank with 10,000mAh for ~2 weeks runtime

## 🔋 Power Consumption & Battery Life

### Current Draw
- **Active broadcasting:** ~20-30mA @ 3.3V
- **Deep sleep (not used):** ~10μA

### Battery Life Estimates

| Battery Capacity | Runtime |
|-----------------|---------|
| 1000mAh | ~1-2 days |
| 5000mAh | ~1 week |
| 10000mAh | ~2 weeks |
| USB Power | Unlimited |

**Tip:** For long-term deployment, use USB power adapters rather than batteries.

## 🛠️ Troubleshooting

### ESP32 Not Detected

**macOS:**
```bash
# List USB devices
ls /dev/cu.*

# Install driver if needed (for CH340 chips)
brew tap adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
brew install --cask wch-ch34x-usb-serial-driver
```

**Linux:**
```bash
# Check device
dmesg | grep tty

# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in
```

**Windows:**
- Install [CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) or [CH340 driver](http://www.wch.cn/downloads/CH341SER_EXE.html)
- Check Device Manager for COM port

### Build Fails

```bash
# Clean build
idf.py fullclean

# Rebuild
idf.py build
```

### Beacon Not Detected by iOS

1. **Check Bluetooth:** Ensure Bluetooth is ON
2. **Check UUID:** Beacon UUID must match iOS app configuration
3. **Check Mode:** Ensure `IBEACON_MODE` is set to `IBEACON_SENDER` in menuconfig
4. **Check Serial:** Run `idf.py monitor` to verify beacon is broadcasting
5. **Check Distance:** Move closer to beacon (within 5m)
6. **Check Power:** Verify ESP32 is powered (LED should be on)

### How to Check IBEACON_MODE

```bash
# Configure project
idf.py menuconfig

# Navigate to: iBeacon Transmitter Configuration
# Verify mode is set to transmitter
# Save and exit: S, then Q
```

## 📱 Using with iOS Room Detector App

1. **Configure UUID in iOS app:**
   - Open Room Detector app
   - Go to Settings → Beacon UUID
   - Enter the same UUID you configured in the ESP32

2. **Create rooms:**
   - Add a new room in the app
   - Scan for beacon
   - Assign the detected beacon to the room

3. **Link to HomeKit (optional):**
   - Import rooms from HomeKit
   - Assign scenes for automatic lighting

4. **Test:**
   - Walk into the room with the beacon
   - App should detect your location within 1-2 seconds

## 🔄 Updating Firmware

To change beacon configuration later:

```bash
# Edit configuration
nano main/main.c

# Rebuild and flash
idf.py build flash

# Monitor (optional)
idf.py monitor
```

## 📊 Advanced: Adjusting for Different Environments

### Large Open Spaces
```c
#define TRANSMIT_POWER 0           // Maximum range
#define ADVERTISING_INTERVAL_MS 1000
```

### Small Rooms (Avoid False Detection)
```c
#define TRANSMIT_POWER -20         // Short range
#define ADVERTISING_INTERVAL_MS 500
```

### Battery Optimization
```c
#define TRANSMIT_POWER -12         // Reduced range
#define ADVERTISING_INTERVAL_MS 2000  // Less frequent
```

## 📚 Additional Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [Apple iBeacon Specification](https://developer.apple.com/ibeacon/)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [BLE Advertising Primer](https://learn.adafruit.com/introduction-to-bluetooth-low-energy/gap)

## 🆘 Getting Help

1. **Check serial output:** `idf.py monitor` shows detailed logs
2. **Verify UUID matches:** iOS app and ESP32 must use same UUID
3. **Test with beacon scanner:** Use free iOS apps like "Beacon Scanner" to verify broadcasting
4. **Check ESP-IDF version:** Run `idf.py --version` (requires v4.4+)

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third-Party Components

- **ESP-IDF**: Apache License 2.0 - Copyright (c) 2015-2021 Espressif Systems
- **iBeacon Protocol**: Trademark of Apple Inc. - [Obtain commercial license](https://developer.apple.com/ibeacon/)

## 🙏 Acknowledgments

- Built on ESP-IDF Bluetooth Low Energy framework by Espressif Systems
- Implements Apple iBeacon specification
- Optimized and enhanced for production use
- Community contributions and feedback

## 📞 Support

- 📖 **Documentation**: Check this README and code comments
- 🐛 **Issues**: [GitHub Issues](https://github.com/yourusername/esp32-ibeacon/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/yourusername/esp32-ibeacon/discussions)
- 📧 **Contact**: Open an issue for support

## ⭐ Show Your Support

If this project helped you, please consider:
- ⭐ Starring the repository
- 🍴 Forking for your own projects
- 📢 Sharing with others who might find it useful
- 💡 Contributing improvements

---

**Built with ❤️ for the ESP32 community**

Made for room detection, presence awareness, and indoor positioning applications.
