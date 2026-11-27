# Smart Bin: ESP32-CAM + YOLO11n

The ESP32-CAM takes a 240×240 photo every few seconds and sends it over Wi-Fi to a
server on your laptop. The server runs the trained YOLO11n model (`.pt`) on the
GPU and replies with what it found (`milk_packet` / `yogurt_cup`).

```
ESP32-CAM ──JPEG (HTTP POST)──► laptop: FastAPI + YOLO11n (RTX 4050)
          ◄──JSON detections───┘
```

```
smartbin/
├── server/                 runs on the laptop
│   ├── app.py              detection API (+ live view page)
│   ├── run_server.sh       starts the server, prints the URL for the ESP32
│   ├── test_client.py      sends a test image, acting as the ESP32
│   ├── requirements.txt
│   └── models/             put smartbin_yolo11n_320.pt here
└── firmware/               PlatformIO project for the AI-Thinker ESP32-CAM
    ├── platformio.ini
    ├── src/main.cpp
    └── include/secrets.example.h   copy to secrets.h
```

## 1. Server (laptop)

```bash
cd smartbin/server
cp ~/Desktop/smartbin/smartbin_yolo11n_320.pt models/

conda activate env_2_ML                # the env you trained in (has torch + CUDA)
pip install -r requirements.txt

chmod +x run_server.sh
./run_server.sh
```

`run_server.sh` prints the exact `SERVER_URL` line to paste into the firmware.

Test it without the ESP32 (in a second terminal):

```bash
python test_client.py /home/naviya-c/Downloads/Smart-Dustbin.v1-smart-dustbin-v1/test/images/<any>.jpg
```

Then let other devices on your Wi-Fi reach it:

```bash
sudo ufw allow 8000/tcp
```

On your phone (same Wi-Fi), open `http://<laptop-ip>:8000/`. If you see `{"status":"ok"...}`, the ESP32 can connect too.

## 2. Firmware (ESP32-CAM)

1. In VS Code, install the **PlatformIO IDE** extension.
2. Open the `firmware/` folder in VS Code.
3. Copy `include/secrets.example.h` to `include/secrets.h`, then fill in your Wi-Fi and the `SERVER_URL` line printed by `run_server.sh`. Keep `USE_HTTPS 0`.
4. Wire the board for flashing:
   - With an **ESP32-CAM-MB** USB board: plug it in.
   - With a **USB-TTL adapter**: 5V→5V, GND→GND, TX→U0R, RX→U0T, and **GPIO0→GND** while uploading.
5. In PlatformIO, click **Upload**. (USB-TTL: remove the GPIO0 wire afterwards and press Reset.)
6. Click **Monitor** (115200 baud). You should see:

```
Wi-Fi OK, ESP32 IP: 192.168.1.73
[6123 bytes, 180 ms] {"count":1,"detections":[{"class":"milk_packet","conf":0.91,...}]}
```

On Linux, if the upload says permission denied on `/dev/ttyUSB0`, run:
`sudo usermod -aG dialout $USER`, then log out and back in.

## 3. Watch it live

Open `http://<laptop-ip>:8000/view` in a browser. It shows the latest ESP32
frame with boxes, refreshing every second.

Every frame the ESP32 sends is saved in `server/debug/`. These are real
deployment images, useful for checking the camera and for labelling into the next
training round. Turn saving off with `SMARTBIN_SAVE=0 ./run_server.sh`.

## Settings

| Where | Setting | Default |
|---|---|---|
| server (env var) | `SMARTBIN_KEY` | `my-secret`, must match `API_KEY` in `secrets.h` |
| server (env var) | `SMARTBIN_CONF` | `0.5`, minimum confidence to report |
| server (env var) | `SMARTBIN_MODEL` | `models/smartbin_yolo11n_320.pt` |
| server (env var) | `SMARTBIN_IMGSZ` | `320`, must match the training size |
| `secrets.h` | `CAPTURE_INTERVAL_S` | `2` seconds between photos |
| `secrets.h` | `USE_HTTPS` | `0` local, `1` for Cloud Run later |

## Troubleshooting

| Symptom | Fix |
|---|---|
| `HTTP error -1 connection refused` | Server not running, wrong IP, or firewall. Check with the phone test above. |
| `HTTP error 401` | `API_KEY` in `secrets.h` doesn't match `SMARTBIN_KEY`. |
| Wi-Fi never connects | ESP32 only supports 2.4 GHz. Check the SSID and password. |
| Phone can't reach laptop | Router has "AP/client isolation" on. Turn it off, or use a phone hotspot for both. |
| `Camera init failed` | Reseat the camera ribbon cable. Use a stable 5V supply (≥ 1 A). |
| Brownout / random resets | Weak power. Don't power it from a thin USB-TTL 3.3V line. |
| Laptop IP changed | Re-run `run_server.sh`, update `SERVER_URL`, upload again (or reserve the IP in your router). |
