.PHONY: build build-builder up down logs restart clean status help

# Build both builder and server images
build:
	@echo "🔨 Building ESP-IDF builder image..."
	docker build -f Dockerfile.builder -t beacon-builder .
	@echo "🔨 Building OTA server image..."
	docker-compose build
	@echo "✅ Build complete!"

# Build just the ESP-IDF builder image
build-builder:
	docker build -f Dockerfile.builder -t beacon-builder .

# Start the OTA server
up: build
	docker-compose up -d
	@echo ""
	@echo "✅ OTA Server is running!"
	@echo "🌐 Web UI: http://localhost:8080"
	@echo "📡 Firmware URL: http://localhost:8080/beacon_firmware.bin"
	@echo "📊 Status API: http://localhost:8080/status"
	@echo ""
	@echo "The server will:"
	@echo "  - Perform initial build on startup"
	@echo "  - Check git every hour for updates"
	@echo "  - Auto-rebuild if changes detected"
	@echo ""
	@echo "To view logs: make logs"
	@echo "To stop: make down"

# Stop the OTA server
down:
	docker-compose down

# View logs
logs:
	docker-compose logs -f ota-server

# Restart the server
restart:
	docker-compose restart

# Get current status
status:
	@curl -s http://localhost:8080/status | python3 -m json.tool || echo "Server not running"

# Trigger manual build
build-firmware:
	@echo "🔨 Triggering manual build..."
	@curl -X POST http://localhost:8080/build

# Clean up everything
clean:
	docker-compose down -v
	docker rmi beacon-builder 2>/dev/null || true
	docker system prune -f

# Help
help:
	@echo "ESP32 OTA Server - Makefile Commands"
	@echo ""
	@echo "  make build           - Build all Docker images"
	@echo "  make build-builder   - Build only the ESP-IDF builder image"
	@echo "  make up              - Start the OTA server"
	@echo "  make down            - Stop the OTA server"
	@echo "  make logs            - View server logs"
	@echo "  make restart         - Restart the server"
	@echo "  make status          - Get current build status"
	@echo "  make build-firmware  - Trigger manual firmware build"
	@echo "  make clean           - Clean up containers and images"
