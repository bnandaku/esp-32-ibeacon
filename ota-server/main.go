package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

const (
	port          = "8080"
	firmwarePath  = "/firmware"
	firmwareFile  = "beacon_firmware.bin"
	projectPath   = "/project"
	gitBranch     = "main"
	checkInterval = 1 * time.Hour
)

type ServerState struct {
	sync.RWMutex
	LastGitCommit  string
	LastBuildTime  time.Time
	LastCheckTime  time.Time
	BuildInProgress bool
	FirmwareSize   int64
	BuildError     string
}

var state = &ServerState{}

func main() {
	// Start git monitor
	go gitMonitor()

	// HTTP handlers
	http.HandleFunc("/beacon_firmware.bin", serveFirmware)
	http.HandleFunc("/health", healthCheck)
	http.HandleFunc("/status", statusHandler)
	http.HandleFunc("/build", manualBuildHandler)
	http.HandleFunc("/", rootHandler)

	log.Printf("🚀 OTA Server starting on port %s", port)
	log.Printf("📁 Project path: %s", projectPath)
	log.Printf("📁 Firmware path: %s", firmwarePath)
	log.Printf("🔄 Git monitor: checking %s branch every %v", gitBranch, checkInterval)
	log.Println("✅ Server ready")

	if err := http.ListenAndServe(":"+port, logRequest(http.DefaultServeMux)); err != nil {
		log.Fatal(err)
	}
}

func gitMonitor() {
	// Initial build on startup
	time.Sleep(5 * time.Second)
	log.Println("🔨 Performing initial build...")
	buildFirmware()

	ticker := time.NewTicker(checkInterval)
	defer ticker.Stop()

	for range ticker.C {
		checkAndBuild()
	}
}

func checkAndBuild() {
	state.Lock()
	state.LastCheckTime = time.Now()
	state.Unlock()

	log.Println("🔍 Checking for git updates...")

	// Get current commit
	currentCommit := getCurrentCommit()

	// Git pull
	cmd := exec.Command("git", "-C", projectPath, "pull", "origin", gitBranch)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Printf("❌ Git pull failed: %v\n%s", err, output)
		return
	}

	log.Printf("📡 Git pull: %s", strings.TrimSpace(string(output)))

	// Check if there are changes
	newCommit := getCurrentCommit()

	if currentCommit != newCommit {
		log.Printf("🆕 New commit detected: %s -> %s", currentCommit[:8], newCommit[:8])
		buildFirmware()
	} else {
		log.Println("✅ No changes detected")
	}
}

func getCurrentCommit() string {
	cmd := exec.Command("git", "-C", projectPath, "rev-parse", "HEAD")
	output, err := cmd.Output()
	if err != nil {
		return "unknown"
	}
	return strings.TrimSpace(string(output))
}

func buildFirmware() {
	state.Lock()
	if state.BuildInProgress {
		state.Unlock()
		log.Println("⚠️  Build already in progress, skipping")
		return
	}
	state.BuildInProgress = true
	state.BuildError = ""
	state.Unlock()

	defer func() {
		state.Lock()
		state.BuildInProgress = false
		state.Unlock()
	}()

	log.Println("🔨 Starting firmware build...")
	startTime := time.Now()

	// Get host project path from environment (fallback to container path)
	hostProjectPath := os.Getenv("HOST_PROJECT_PATH")
	if hostProjectPath == "" {
		hostProjectPath = projectPath
	}

	// Run build in Docker container
	cmd := exec.Command("docker", "run", "--rm",
		"-v", hostProjectPath+":/project",
		"-v", "ota-server_firmware-data:/firmware",
		"beacon-builder",
		"/build.sh")

	output, err := cmd.CombinedOutput()
	buildDuration := time.Since(startTime)

	if err != nil {
		errMsg := fmt.Sprintf("Build failed after %v: %v\n%s", buildDuration, err, output)
		log.Printf("❌ %s", errMsg)
		state.Lock()
		state.BuildError = errMsg
		state.Unlock()
		return
	}

	// Update state
	state.Lock()
	state.LastBuildTime = time.Now()
	state.LastGitCommit = getCurrentCommit()

	// Get firmware size
	firmwareFullPath := filepath.Join(firmwarePath, firmwareFile)
	if fileInfo, err := os.Stat(firmwareFullPath); err == nil {
		state.FirmwareSize = fileInfo.Size()
	}
	state.Unlock()

	log.Printf("✅ Build completed in %v", buildDuration)
	log.Printf("📦 Firmware size: %.2f KB", float64(state.FirmwareSize)/1024)
}

func serveFirmware(w http.ResponseWriter, r *http.Request) {
	fullPath := filepath.Join(firmwarePath, firmwareFile)

	fileInfo, err := os.Stat(fullPath)
	if os.IsNotExist(err) {
		log.Printf("❌ Firmware file not found: %s", fullPath)
		http.Error(w, "Firmware not found", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%s", firmwareFile))
	w.Header().Set("Content-Length", fmt.Sprintf("%d", fileInfo.Size()))

	log.Printf("📤 Serving firmware: %s (%.2f KB) to %s", firmwareFile, float64(fileInfo.Size())/1024, r.RemoteAddr)
	http.ServeFile(w, r, fullPath)
	log.Printf("✅ Firmware delivered")
}

func healthCheck(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, "OK")
}

func statusHandler(w http.ResponseWriter, r *http.Request) {
	state.RLock()
	defer state.RUnlock()

	w.Header().Set("Content-Type", "application/json")
	fmt.Fprintf(w, `{
  "lastCommit": "%s",
  "lastBuild": "%s",
  "lastCheck": "%s",
  "buildInProgress": %v,
  "firmwareSize": %d,
  "buildError": "%s"
}`, state.LastGitCommit, state.LastBuildTime.Format(time.RFC3339),
		state.LastCheckTime.Format(time.RFC3339), state.BuildInProgress,
		state.FirmwareSize, state.BuildError)
}

func manualBuildHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	go buildFirmware()

	fmt.Fprintf(w, "Build triggered\n")
}

func rootHandler(w http.ResponseWriter, r *http.Request) {
	state.RLock()
	defer state.RUnlock()

	fullPath := filepath.Join(firmwarePath, firmwareFile)
	fileInfo, _ := os.Stat(fullPath)

	buildStatus := "⏳ Never built"
	if !state.LastBuildTime.IsZero() {
		buildStatus = fmt.Sprintf("✅ Built %s", state.LastBuildTime.Format("2006-01-02 15:04:05"))
	}
	if state.BuildInProgress {
		buildStatus = "🔨 Build in progress..."
	}
	if state.BuildError != "" {
		buildStatus = fmt.Sprintf("❌ Build failed: %s", state.BuildError)
	}

	firmwareStatus := "❌ Not found"
	if fileInfo != nil {
		firmwareStatus = fmt.Sprintf("✅ %.2f KB (modified %s)",
			float64(fileInfo.Size())/1024,
			fileInfo.ModTime().Format("2006-01-02 15:04:05"))
	}

	html := fmt.Sprintf(`<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Server</title>
    <style>
        body { font-family: system-ui; max-width: 900px; margin: 50px auto; padding: 20px; }
        .status { padding: 20px; border-radius: 8px; margin: 20px 0; background: #f5f5f5; border: 1px solid #ddd; }
        h1 { color: #333; }
        .info { margin: 10px 0; }
        .label { font-weight: bold; min-width: 150px; display: inline-block; }
        a { color: #007bff; text-decoration: none; }
        a:hover { text-decoration: underline; }
        button { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; }
        button:hover { background: #0056b3; }
    </style>
    <script>
        function triggerBuild() {
            fetch('/build', {method: 'POST'})
                .then(r => r.text())
                .then(data => {
                    alert('Build triggered! Refresh page in a minute to see results.');
                });
        }
        setTimeout(() => location.reload(), 30000); // Auto-refresh every 30s
    </script>
</head>
<body>
    <h1>🚀 ESP32 Beacon OTA Server</h1>

    <div class="status">
        <h2>Status</h2>
        <div class="info"><span class="label">Build Status:</span> %s</div>
        <div class="info"><span class="label">Firmware:</span> %s</div>
        <div class="info"><span class="label">Last Git Commit:</span> %s</div>
        <div class="info"><span class="label">Last Check:</span> %s</div>
        <div class="info"><span class="label">Next Check:</span> in ~%d minutes</div>
    </div>

    <div class="status">
        <h2>Actions</h2>
        <button onclick="triggerBuild()">🔨 Trigger Build Now</button>
        <a href="/beacon_firmware.bin" style="margin-left: 20px;">📥 Download Firmware</a>
        <a href="/status" style="margin-left: 20px;">📊 JSON Status</a>
    </div>

    <div class="status">
        <h2>Configuration</h2>
        <div class="info"><span class="label">Git Branch:</span> %s</div>
        <div class="info"><span class="label">Check Interval:</span> %v</div>
        <div class="info"><span class="label">Beacon Check:</span> Every 5 minutes</div>
    </div>

    <p><small>Page auto-refreshes every 30 seconds</small></p>
</body>
</html>`, buildStatus, firmwareStatus, state.LastGitCommit[:min(8, len(state.LastGitCommit))],
		state.LastCheckTime.Format("2006-01-02 15:04:05"),
		int(time.Until(state.LastCheckTime.Add(checkInterval)).Minutes()),
		gitBranch, checkInterval)

	w.Header().Set("Content-Type", "text/html")
	fmt.Fprint(w, html)
}

func logRequest(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		handler.ServeHTTP(w, r)
		log.Printf("📍 %s %s from %s (took %v)", r.Method, r.URL.Path, r.RemoteAddr, time.Since(start))
	})
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
