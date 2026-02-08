#!/usr/bin/env python3
"""
Beacon Identifier

This script reads the serial output from a connected beacon
and displays its Major/Minor configuration.

Usage:
    python3 identify_beacon.py /dev/cu.usbserial-0001
"""

import sys
import serial
import time
import re

def identify_beacon(port, baud=115200, timeout=10):
    """Read beacon configuration from serial port"""
    print(f"🔍 Identifying beacon on {port}...")
    print("   (Press the RESET button on the beacon if nothing appears)\n")

    try:
        ser = serial.Serial(port, baud, timeout=1)
        time.sleep(0.5)  # Wait for port to open

        start_time = time.time()
        major = None
        minor = None
        uuid = None
        device_name = None

        while time.time() - start_time < timeout:
            if ser.in_waiting:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()

                    # Look for configuration lines
                    if 'MAJOR:' in line:
                        match = re.search(r'MAJOR:\s*(\d+)', line)
                        if match:
                            major = int(match.group(1))
                            print(f"   Found MAJOR: {major}")

                    if 'MINOR:' in line:
                        match = re.search(r'MINOR:\s*(\d+)', line)
                        if match:
                            minor = int(match.group(1))
                            print(f"   Found MINOR: {minor}")

                    if 'UUID:' in line:
                        match = re.search(r'UUID:\s*([0-9A-Fa-f\-]+)', line)
                        if match:
                            uuid = match.group(1)
                            print(f"   Found UUID: {uuid}")

                    if 'Device name set to:' in line:
                        match = re.search(r'Device name set to:\s*(.+)', line)
                        if match:
                            device_name = match.group(1)
                            print(f"   BLE Name: {device_name}")

                    # If we found all info, we're done
                    if major is not None and minor is not None:
                        break

                except UnicodeDecodeError:
                    continue

        ser.close()

        # Display results
        print("\n" + "="*70)
        if major is not None and minor is not None:
            print("  ✅ BEACON IDENTIFIED")
            print("="*70)
            print(f"  UUID:      {uuid or 'ED17A803-D1AC-4F04-A2F0-7802B4C9C70C'}")
            print(f"  MAJOR:     {major}")
            print(f"  MINOR:     {minor}")
            if device_name:
                print(f"  BLE Name:  {device_name}")
            print("="*70)

            # Suggest room name
            room_map = {
                10: "Living Room",
                11: "Bedroom",
                12: "Office",
                13: "Hallway",
                14: "Kitchen"
            }
            if minor in room_map:
                print(f"  📍 This is the {room_map[minor]} beacon")
            print("="*70)
            return True
        else:
            print("="*70)
            print("  ⚠️  Could not identify beacon")
            print("="*70)
            print("  Try:")
            print("  1. Press the RESET button on the beacon")
            print("  2. Disconnect and reconnect USB")
            print("  3. Run the script again")
            print("="*70)
            return False

    except serial.SerialException as e:
        print(f"❌ Error: {e}")
        print(f"   Make sure {port} is the correct port")
        return False
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 identify_beacon.py <serial_port>")
        print("\nExample:")
        print("  python3 identify_beacon.py /dev/cu.usbserial-0001")
        print("\nTo find available ports:")
        print("  ls /dev/cu.* | grep usb")
        sys.exit(1)

    port = sys.argv[1]
    identify_beacon(port)

if __name__ == '__main__':
    main()
