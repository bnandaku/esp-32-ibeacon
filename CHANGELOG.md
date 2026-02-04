# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-02-04

### Added
- Initial release of ESP32 iBeacon Transmitter
- Simple string-based UUID configuration (copy-paste friendly)
- Configurable transmission power (-127 to 0 dBm)
- Adjustable advertising interval (100-10000ms)
- Automatic UUID string to byte array conversion
- Low power optimized (~20-30mA consumption)
- Professional code structure and documentation
- Comprehensive README with setup guide
- Troubleshooting section
- Multiple beacon configuration examples
- Battery life estimates
- Beacon placement tips

### Features
- Support for ESP32, ESP32-C3, ESP32-S3, and other variants
- Compatible with iOS Room Detector app
- Works with any iBeacon-compatible device
- Clean, commented, production-ready code
- MIT License for free use

### Documentation
- Complete setup guide
- ESP-IDF installation instructions
- Configuration options explained
- Troubleshooting guide
- Contributing guidelines
- Issue and PR templates

---

## Release Notes

### Version 1.0.0
This is the first stable release of the ESP32 iBeacon Transmitter firmware. Professional-grade implementation optimized for user-friendliness and production deployment.

**Key Highlights:**
- No more hex byte array confusion - just paste UUID strings!
- Clear configuration section at the top of main.c
- Optimized for room detection and presence awareness
- Professional code quality suitable for commercial use
- Comprehensive documentation and support

**Tested On:**
- ESP32-DevKitC
- ESP32-C3-DevKitM-1
- ESP32-S3-DevKitC-1

**Compatible With:**
- ESP-IDF v4.4 and newer
- iOS Room Detector app
- Generic beacon scanner apps

---

[1.0.0]: https://github.com/yourusername/esp32-ibeacon/releases/tag/v1.0.0
