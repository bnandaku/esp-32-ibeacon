#!/usr/bin/env python3
"""
Automated ESP32 Webhook Sender Flasher

This script automates the process of building and flashing ESP32 webhook sender firmware.

Usage:
    python3 flash_beacon.py --port /dev/cu.usbserial-0001
    python3 flash_beacon.py --port /dev/cu.usbserial-0001 --baud 115200
"""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

class WebhookFlasher:
    def __init__(self, project_dir="/Users/bharat/esp32/BluetoothBeacon"):
        self.project_dir = Path(project_dir)
        self.main_c_path = self.project_dir / "main" / "main.c"
        self.idf_activate = ". /Users/bharat/.espressif/tools/activate_idf_v5.5.2.sh >/dev/null 2>&1"
        self.idf_py = "/Users/bharat/.espressif/v5.5.2/esp-idf/tools/idf.py"

    def build_firmware(self):
        """Build the firmware"""
        print("🔨 Building firmware...")

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.idf_py} build'"
        result = subprocess.run(cmd, shell=True, capture_output=False, text=True)

        if result.returncode != 0:
            print("❌ Build failed!")
            return False

        print("✓ Firmware built successfully")
        return True

    def flash_device(self, port, baud=115200):
        """Flash the firmware to the device"""
        print(f"⚡ Flashing device on {port} at {baud} baud...")

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.idf_py} -p {port} -b {baud} flash'"
        result = subprocess.run(cmd, shell=True, capture_output=False, text=True)

        if result.returncode != 0:
            print("❌ Flash failed!")
            return False

        print(f"✓ Device flashed successfully on {port}")
        return True

    def update_default_config(self, major, minor):
        """Update DEFAULT_BEACON_MAJOR and DEFAULT_BEACON_MINOR in main.c"""
        print(f"📝 Updating beacon config: Major={major}, Minor={minor}")

        with open(self.main_c_path, 'r') as f:
            content = f.read()

        # Update MAJOR
        content = re.sub(
            r'#define DEFAULT_BEACON_MAJOR \d+',
            f'#define DEFAULT_BEACON_MAJOR {major}',
            content
        )

        # Update MINOR
        content = re.sub(
            r'#define DEFAULT_BEACON_MINOR \d+',
            f'#define DEFAULT_BEACON_MINOR {minor}',
            content
        )

        with open(self.main_c_path, 'w') as f:
            f.write(content)

        print(f"✓ Updated config in {self.main_c_path}")

    def flash_single(self, major, minor, port, baud=115200):
        """Flash a single beacon with specific major/minor values"""
        print("\n" + "="*70)
        print(f"  Flashing Beacon - Major: {major}, Minor: {minor}")
        print(f"  Port: {port}, Baud: {baud}")
        print("="*70)

        # Update config
        self.update_default_config(major, minor)

        # Build firmware
        if not self.build_firmware():
            return False

        # Flash device
        if not self.flash_device(port, baud):
            return False

        print("\n✅ Beacon ready!")
        print(f"   Major: {major}, Minor: {minor}")
        print("="*70)

        return True

    def monitor(self, port, baud=115200):
        """Monitor serial output"""
        print(f"📡 Starting serial monitor on {port}...")
        print("   Press Ctrl+] to exit monitor")

        cmd = f"bash -c 'cd {self.project_dir} && {self.idf_activate} && {self.idf_py} -p {port} monitor'"
        subprocess.run(cmd, shell=True)

    def flash_and_monitor(self, port, baud=115200):
        """Build, flash, and monitor the device"""
        print("\n" + "="*70)
        print(f"  ESP32 Webhook Sender - Build & Flash")
        print(f"  Port: {port}, Baud: {baud}")
        print("="*70)

        # Build firmware
        if not self.build_firmware():
            return False

        # Flash device
        if not self.flash_device(port, baud):
            return False

        print("\n✅ Device ready!")
        print("   Firmware: ESP32 Webhook Sender v3.0.0")
        print("   Connects to WiFi and sends webhooks")
        print("="*70)

        return True

    def flash_batch(self, config_file):
        """Flash multiple beacons from a JSON config file"""
        print("\n" + "="*70)
        print("  Batch Beacon Flashing")
        print("="*70)

        with open(config_file, 'r') as f:
            beacons = json.load(f)

        total = len(beacons)
        success_count = 0

        for i, beacon in enumerate(beacons, 1):
            major = beacon['major']
            minor = beacon['minor']
            port = beacon['port']
            name = beacon.get('name', f'Beacon {i}')

            print(f"\n[{i}/{total}] {name}")

            # Each beacon builds with its own config
            if self.flash_single(major, minor, port):
                success_count += 1
            else:
                print(f"❌ Failed to flash {name}")
                response = input("\nContinue with next beacon? (y/n): ")
                if response.lower() != 'y':
                    break

            if i < total:
                input("\n⏸️  Connect next beacon and press Enter to continue...")

        print("\n" + "="*70)
        print(f"  Batch Complete: {success_count}/{total} beacons flashed")
        print("="*70)

def create_example_config():
    """Create an example beacons.json file"""
    example = [
        {
            "name": "Office",
            "major": 100,
            "minor": 12,
            "port": "/dev/cu.usbserial-0001"
        },
        {
            "name": "Living Room",
            "major": 100,
            "minor": 1,
            "port": "/dev/cu.usbserial-0001"
        },
        {
            "name": "Bedroom",
            "major": 100,
            "minor": 2,
            "port": "/dev/cu.usbserial-0001"
        },
        {
            "name": "Kitchen",
            "major": 100,
            "minor": 3,
            "port": "/dev/cu.usbserial-0001"
        },
        {
            "name": "Bathroom",
            "major": 100,
            "minor": 4,
            "port": "/dev/cu.usbserial-0001"
        }
    ]

    with open('beacons.json', 'w') as f:
        json.dump(example, f, indent=2)

    print("✓ Created example beacons.json file")
    print("  Edit this file with your beacon configurations and run:")
    print("  python3 flash_beacon.py --batch beacons.json")

def main():
    parser = argparse.ArgumentParser(
        description='Automated ESP32 Beacon Flasher',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Flash a single beacon
  python3 flash_beacon.py --major 100 --minor 1 --port /dev/cu.usbserial-0001

  # Flash multiple beacons from config file
  python3 flash_beacon.py --batch beacons.json

  # Create example config file
  python3 flash_beacon.py --create-example
        """
    )

    parser.add_argument('--major', type=int, help='Major number (0-65535)')
    parser.add_argument('--minor', type=int, help='Minor number (0-65535)')
    parser.add_argument('--port', help='Serial port (e.g., /dev/cu.usbserial-0001)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--batch', help='JSON file with multiple beacon configs')
    parser.add_argument('--create-example', action='store_true', help='Create example beacons.json')

    args = parser.parse_args()

    if args.create_example:
        create_example_config()
        return 0

    flasher = WebhookFlasher()

    if args.batch:
        # Batch mode
        if not os.path.exists(args.batch):
            print(f"❌ Config file not found: {args.batch}")
            return 1

        flasher.flash_batch(args.batch)

    elif args.major is not None and args.minor is not None and args.port:
        # Single beacon mode
        if not flasher.flash_single(args.major, args.minor, args.port, args.baud):
            return 1

    else:
        parser.print_help()
        return 1

    return 0

if __name__ == '__main__':
    sys.exit(main())
