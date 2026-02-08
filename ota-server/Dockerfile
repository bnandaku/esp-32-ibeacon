# Build stage
FROM golang:1.22-alpine AS builder

WORKDIR /app

# Copy go mod files
COPY go.* ./
RUN go mod download 2>/dev/null || true

# Copy source code
COPY main.go .

# Build the application
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o ota-server main.go

# Runtime stage
FROM alpine:latest

RUN apk --no-cache add \
    ca-certificates \
    git \
    docker-cli \
    wget

WORKDIR /root/

# Copy the binary from builder
COPY --from=builder /app/ota-server .

# Create firmware directory
RUN mkdir -p /firmware

# Expose port
EXPOSE 8080

# Run the server
CMD ["./ota-server"]
