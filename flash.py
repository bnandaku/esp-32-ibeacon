#!/usr/bin/env python3
"""
Simple ESP32 Flasher

Builds and flashes the ESP32 firmware without modifying any configuration.

Usage:
    python3 flash.py --port /dev/cu.usbserial-0001
    python3 flash.py --port /dev/cu.usbserial-0001 --baud 115200 --monitor
"""

import argparse
import subprocess
import sys
from pathlib import Path

class ESP32Flasher:
    def __init__(self, project_dir="/Users/bharat/esp32/BluetoothBeacon"):
        self.project_dir = Path(project_dir)
        self.idf_activate = ". /Users/bharat/.espressif/tools/activate_idf_v5.5.2.sh"
        self.python = "/Users/bharat/.espressif/tools/python/v5.5.2/venv/bin/python"
        self.idf_py = "/Users/bharat/.espressif/v5.5.2/esp-idf/tools/idf.py"

    def build(self):
        """Build the firmware"""
        print("🔨 Building firmware...")
        print("="*70)

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.python} {self.idf_py} build'"
        result = subprocess.run(cmd, shell=True)

        if result.returncode != 0:
            print("\n❌ Build failed!")
            return False

        print("\n✓ Firmware built successfully")
        return True

    def flash(self, port, baud=115200):
        """Flash the firmware"""
        print(f"\n⚡ Flashing to {port} at {baud} baud...")
        print("="*70)

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.python} {self.idf_py} -p {port} -b {baud} flash'"
        result = subprocess.run(cmd, shell=True)

        if result.returncode != 0:
            print("\n❌ Flash failed!")
            return False

        print("\n✓ Flash completed successfully")
        return True

    def monitor(self, port):
        """Start serial monitor"""
        print(f"\n📡 Starting serial monitor on {port}...")
        print("="*70)
        print("Press Ctrl+] to exit\n")

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.python} {self.idf_py} -p {port} monitor'"
        subprocess.run(cmd, shell=True)

def main():
    parser = argparse.ArgumentParser(
        description='Simple ESP32 Build & Flash Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build and flash
  python3 flash.py --port /dev/cu.usbserial-0001

  # Build, flash, and monitor
  python3 flash.py --port /dev/cu.usbserial-0001 --monitor

  # Use slower baud rate (more reliable on bad cables)
  python3 flash.py --port /dev/cu.usbserial-0001 --baud 115200

  # Use default faster baud rate
  python3 flash.py --port /dev/cu.usbserial-0001
        """
    )

    parser.add_argument('--port', required=True, help='Serial port (e.g., /dev/cu.usbserial-0001)')
    parser.add_argument('--baud', type=int, default=460800, help='Baud rate for flashing (default: 460800)')
    parser.add_argument('--monitor', action='store_true', help='Start serial monitor after flashing')
    parser.add_argument('--skip-build', action='store_true', help='Skip build, just flash')

    args = parser.parse_args()

    flasher = ESP32Flasher()

    # Build
    if not args.skip_build:
        if not flasher.build():
            return 1

    # Flash
    if not flasher.flash(args.port, args.baud):
        return 1

    # Monitor
    if args.monitor:
        flasher.monitor(args.port)

    print("\n" + "="*70)
    print("✅ Done!")
    print("="*70)

    return 0

if __name__ == '__main__':
    sys.exit(main())
