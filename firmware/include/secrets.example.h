#pragma once
// Copy this file to secrets.h (same folder) and fill in your values.
// secrets.h is in .gitignore so your Wi-Fi password never goes to GitHub.

#define WIFI_SSID  "your-wifi-name"      // 2.4 GHz network (ESP32 has no 5 GHz)
#define WIFI_PASS  "your-wifi-password"

// Local laptop test: run server/run_server.sh, it prints the exact line to paste.
#define SERVER_URL "http://192.168.1.50:8000/detect"
#define API_KEY    "my-secret"           // must match SMARTBIN_KEY on the server

// 0 = plain http (local laptop), 1 = https (Cloud Run later)
#define USE_HTTPS  0

// Seconds between photos
#define CAPTURE_INTERVAL_S 2
