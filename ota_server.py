#!/usr/bin/env python3
"""
Simple OTA Server for ESP32 iBeacon firmware updates

This server hosts the firmware binary and serves it to all beacons
that request updates. All beacons will automatically download and
install the new firmware.

Usage:
    1. Build your firmware: idf.py build
    2. Copy firmware to server: cp build/BluetoothBeacon.bin firmware/beacon_firmware.bin
    3. Start server: python3 ota_server.py
    4. All beacons will auto-update within 5 minutes

The server will serve firmware from the 'firmware' directory.
"""

import http.server
import socketserver
import os
import sys
from datetime import datetime

PORT = 8080
FIRMWARE_DIR = "firmware"
FIRMWARE_FILE = "beacon_firmware.bin"

class OTAHandler(http.server.SimpleHTTPRequestHandler):
    """Custom HTTP handler for OTA updates"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FIRMWARE_DIR, **kwargs)

    def log_message(self, format, *args):
        """Custom log format with timestamp"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        client_ip = self.client_address[0]
        print(f"[{timestamp}] {client_ip} - {format % args}")

    def do_GET(self):
        """Handle GET requests"""
        if self.path == f"/{FIRMWARE_FILE}":
            firmware_path = os.path.join(FIRMWARE_DIR, FIRMWARE_FILE)

            if not os.path.exists(firmware_path):
                self.send_error(404, "Firmware not found")
                print(f"❌ ERROR: Firmware file not found: {firmware_path}")
                return

            file_size = os.path.getsize(firmware_path)
            print(f"✓ Serving firmware update ({file_size} bytes) to {self.client_address[0]}")

        return super().do_GET()

def main():
    """Start the OTA server"""

    # Create firmware directory if it doesn't exist
    if not os.path.exists(FIRMWARE_DIR):
        os.makedirs(FIRMWARE_DIR)
        print(f"Created firmware directory: {FIRMWARE_DIR}")

    # Check if firmware file exists
    firmware_path = os.path.join(FIRMWARE_DIR, FIRMWARE_FILE)
    if not os.path.exists(firmware_path):
        print("=" * 70)
        print("⚠️  WARNING: Firmware file not found!")
        print("=" * 70)
        print(f"\nTo prepare firmware for OTA update:")
        print(f"  1. Build firmware: idf.py build")
        print(f"  2. Copy to server: cp build/BluetoothBeacon.bin {firmware_path}")
        print(f"  3. Restart this server\n")
        print("Server will start but updates will fail until firmware is available.")
        print("=" * 70)
        input("\nPress Enter to start server anyway, or Ctrl+C to exit...")
    else:
        file_size = os.path.getsize(firmware_path)
        print("=" * 70)
        print("✓ Firmware found!")
        print("=" * 70)
        print(f"File: {firmware_path}")
        print(f"Size: {file_size:,} bytes ({file_size / 1024:.1f} KB)")
        print("=" * 70)

    # Get local IP address
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        local_ip = s.getsockname()[0]
    except:
        local_ip = '127.0.0.1'
    finally:
        s.close()

    # Start server
    with socketserver.TCPServer(("", PORT), OTAHandler) as httpd:
        print("\n" + "=" * 70)
        print("  ESP32 iBeacon OTA Update Server")
        print("=" * 70)
        print(f"\n✓ Server running on:")
        print(f"  - Local:   http://localhost:{PORT}/{FIRMWARE_FILE}")
        print(f"  - Network: http://{local_ip}:{PORT}/{FIRMWARE_FILE}")
        print(f"\n✓ Configure your beacons with:")
        print(f"  - menuconfig -> WiFi SSID and Password")
        print(f"  - menuconfig -> Firmware URL: http://{local_ip}:{PORT}/{FIRMWARE_FILE}")
        print(f"\n✓ All beacons will check for updates every 5 minutes")
        print(f"\n⚠️  Press Ctrl+C to stop server")
        print("=" * 70)
        print()

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\n" + "=" * 70)
            print("Server stopped.")
            print("=" * 70)
            sys.exit(0)

if __name__ == "__main__":
    main()
