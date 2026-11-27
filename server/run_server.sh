#!/usr/bin/env bash
# Start the Smart Bin server so the ESP32 on your Wi-Fi can reach it.
cd "$(dirname "$0")"

export SMARTBIN_KEY="${SMARTBIN_KEY:-my-secret}"   # must match API_KEY in firmware/include/secrets.h
PORT="${PORT:-8000}"

IP=$(hostname -I 2>/dev/null | awk '{print $1}')
echo "------------------------------------------------------------"
echo " Put this in firmware/include/secrets.h:"
echo "   #define SERVER_URL \"http://${IP}:${PORT}/detect\""
echo "   #define API_KEY    \"${SMARTBIN_KEY}\""
echo " Live view in a browser:  http://${IP}:${PORT}/view"
echo "------------------------------------------------------------"

exec uvicorn app:app --host 0.0.0.0 --port "$PORT"
