# ESP32 Beacon OTA Server

Docker-based OTA (Over-The-Air) update server for ESP32 beacons with automatic git monitoring and firmware compilation.

## Features

- ✅ **Automatic Git Monitoring**: Pulls from `main` branch every hour
- ✅ **Auto-Build on Changes**: Detects commits and rebuilds firmware automatically
- ✅ **Dockerized Build Environment**: ESP-IDF v5.5.2 in isolated container
- ✅ **Go HTTP Server**: Lightweight, efficient firmware delivery
- ✅ **Web UI**: Monitor build status, trigger manual builds
- ✅ **Health Monitoring**: Built-in health checks and status API
- ✅ **NVS Configuration**: Beacons store Major/Minor persistently

## Architecture

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────┐
│   Git Repo      │────▶│  OTA Server (Go) │────▶│  ESP32      │
│   (main branch) │     │  - Git monitor   │     │  Beacons    │
│                 │     │  - Auto-build    │     │  (5 min     │
└─────────────────┘     │  - HTTP server   │     │   check)    │
                        └────────┬─────────┘     └─────────────┘
                                 │
                                 ▼
                        ┌──────────────────┐
                        │  Builder         │
                        │  (ESP-IDF v5.5.2)│
                        │  Docker Container│
                        └──────────────────┘
```

## Quick Start

### 1. Start the OTA Server

```bash
cd ota-server
make up
```

This will:
- Build the ESP-IDF builder Docker image
- Build the OTA server Docker image
- Start the server
- Perform an initial firmware build
- Begin monitoring git every hour

### 2. Access the Web UI

Open http://localhost:8080 to see:
- Current build status
- Last git commit
- Firmware info
- Manual build trigger

### 3. Configure Beacon WiFi

Update WiFi credentials in menuconfig (one-time setup):

```bash
cd /Users/bharat/esp32/BluetoothBeacon
idf.py menuconfig
```

Set:
- **Example Configuration** → WiFi SSID: `Your_Network_Name`
- **Example Configuration** → WiFi Password: `Your_Password`
- **Example Configuration** → Firmware URL: `http://YOUR_COMPUTER_IP:8080/beacon_firmware.bin`

**Important:** Replace `YOUR_COMPUTER_IP` with your actual IP address (use `ifconfig` or `ipconfig`), NOT `localhost`.

### 4. Flash Beacons (Initial Setup)

Flash each beacon via USB with their Major/Minor configuration:

```bash
cd /Users/bharat/esp32/BluetoothBeacon
python3 flash_beacon.py --major 100 --minor 10 --port /dev/cu.usbserial-0001
```

The script will:
1. Flash the generic firmware
2. Send `CONFIG:100:10` command via serial
3. Beacon saves to NVS and reboots
4. Beacon connects to WiFi and checks for updates every 5 minutes

Repeat for all beacons:
- Living Room: `--minor 10`
- Bedroom: `--minor 11`
- Office: `--minor 12`
- Hallway: `--minor 13`
- Kitchen: `--minor 14`

### 5. Deploy Updates (Automatic)

After initial setup, updates are fully automatic:

```bash
# 1. Make changes to your code (e.g., adjust transmit power in main.c)
git add main/main.c
git commit -m "Adjust transmit power to +6 dBm"
git push origin main

# 2. Wait up to 1 hour (or trigger manual build)
# 3. Server detects changes and rebuilds
# 4. Beacons auto-update within 5 minutes!
```

## Makefile Commands

```bash
make build           # Build all Docker images
make up              # Start OTA server
make down            # Stop OTA server
make logs            # View server logs (live)
make status          # Get current build status (JSON)
make build-firmware  # Trigger manual build immediately
make restart         # Restart server
make clean           # Remove all containers and images
```

## How It Works

### Git Monitoring
- Server checks git every **1 hour**
- Runs `git pull origin main`
- Compares commit SHA before/after
- If changed → triggers build

### Build Process
1. Server executes builder Docker container
2. Builder mounts project directory
3. Runs `idf.py build` inside ESP-IDF environment
4. Copies firmware to `/firmware` volume
5. Server immediately starts serving new firmware

### Beacon Updates
- Beacons check `http://YOUR_IP:8080/beacon_firmware.bin` every **5 minutes**
- If newer version available → download and flash
- Beacon reboots with new firmware
- **Major/Minor preserved** (stored in NVS, not in firmware)

## Configuration Persistence

Beacons store their Major/Minor in **NVS (Non-Volatile Storage)**:

- ✅ Survives OTA updates
- ✅ Survives power loss
- ✅ Can be reconfigured via serial: `CONFIG:<major>:<minor>`

**To change a beacon's config:**
```bash
# Connect via serial
screen /dev/cu.usbserial-0001 115200

# Send command
CONFIG:100:15

# Beacon saves and reboots
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web UI dashboard |
| `/beacon_firmware.bin` | GET | Download firmware |
| `/status` | GET | JSON status (build time, commit, etc.) |
| `/health` | GET | Health check (returns "OK") |
| `/build` | POST | Trigger manual build |

## Troubleshooting

### Server won't start
```bash
# Check logs
make logs

# Rebuild images
make clean
make build
make up
```

### Beacons not updating
1. **Check server is running**: `make status`
2. **Verify firmware exists**: Check Web UI at http://localhost:8080
3. **Check beacon WiFi**: View serial output, ensure connected
4. **Verify IP address**: Must use computer's IP, not localhost
5. **Check firewall**: Ensure port 8080 is accessible

### Manual build not working
```bash
# Trigger build via curl
curl -X POST http://localhost:8080/build

# Check logs
make logs

# If builder image is missing
make build-builder
```

### Git pull failing
```bash
# Check if project is a git repo
cd /Users/bharat/esp32/BluetoothBeacon
git status

# Ensure on main branch
git checkout main

# Check remote
git remote -v
```

## Advanced Configuration

### Change check interval
Edit `main.go`:
```go
const checkInterval = 30 * time.Minute  // Check every 30 min
```
Rebuild: `make down && make up`

### Use different git branch
Edit `main.go`:
```go
const gitBranch = "development"
```
Rebuild: `make down && make up`

### Change server port
Edit `docker-compose.yml`:
```yaml
ports:
  - "8888:8080"  # External port 8888
```
Restart: `make restart`

## Production Deployment

For production, consider:

1. **Use HTTPS**: Add nginx reverse proxy with SSL
2. **Authentication**: Add basic auth to `/build` endpoint
3. **Monitoring**: Use Prometheus/Grafana for metrics
4. **Backup**: Backup firmware directory regularly
5. **Rate Limiting**: Prevent too many beacon requests

## Security Notes

- ⚠️ Server has Docker socket access (needs to run builder)
- ⚠️ Firmware is served over HTTP (consider HTTPS for production)
- ⚠️ No authentication on build trigger (add if exposed publicly)
- ✅ Builder runs in isolated container
- ✅ Project mounted read-only for server

## File Structure

```
ota-server/
├── main.go              # OTA server (Go)
├── Dockerfile           # Server container
├── Dockerfile.builder   # ESP-IDF builder container
├── build.sh             # Build script for firmware
├── docker-compose.yml   # Orchestration
├── Makefile             # Convenience commands
└── README.md            # This file
```

## License

MIT
