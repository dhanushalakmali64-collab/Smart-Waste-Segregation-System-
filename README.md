# Smart Bin: ESP32-CAM + YOLO11n Object Detection

A smart dustbin that recognises **milk packets** and **yoghurt cups**.

An **ESP32-CAM** takes a photo every few seconds and sends it over Wi-Fi to your **computer**.
Your computer runs a YOLO11n AI model on the photo and replies with what it found.

```
 ┌─────────────┐   photo (Wi-Fi)    ┌──────────────────────────┐
 │  ESP32-CAM  │ ─────────────────► │  Your computer (server)  │
 │  (camera)   │ ◄───────────────── │  runs the YOLO11n model  │
 └─────────────┘   "milk_packet"    └──────────────────────────┘
```

- Works with or without a graphics card (GPU). A normal CPU is fast enough.
- Works on **Linux, macOS and Windows**.
- No cloud account needed: everything runs on your own Wi-Fi.

---

## Contents

1. [What you need](#1-what-you-need)
2. [Folder layout](#2-folder-layout)
3. [Set up the server (your computer)](#3-set-up-the-server-your-computer)
4. [Test the server without the ESP32](#4-test-the-server-without-the-esp32)
5. [Set up the ESP32-CAM](#5-set-up-the-esp32-cam)
6. [Use it](#6-use-it)
7. [Starting it again next time](#7-starting-it-again-next-time)
8. [Settings you can change](#8-settings-you-can-change)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. What you need

**Hardware**

| Item | Notes |
|---|---|
| AI-Thinker **ESP32-CAM** (with OV2640 camera) | The common, cheap ESP32 camera board |
| **ESP32-CAM-MB** USB adapter *(recommended)* | Or a USB-TTL adapter + jumper wires |
| USB cable / 5 V power supply, **at least 1 A** | Weak power causes random resets |
| A computer on the **same Wi-Fi** as the ESP32 | This runs the AI model |
| A **2.4 GHz** Wi-Fi network | The ESP32 cannot use 5 GHz networks |

**Software**

| Item | Get it from |
|---|---|
| Python 3.10 or newer | https://www.python.org/downloads/ |
| VS Code | https://code.visualstudio.com/ |
| PlatformIO IDE (VS Code extension) | Install from inside VS Code (step 5.1) |

**The trained model file**

| File | Where it goes |
|---|---|
| `smartbin_yolo11n_320.pt` | `server/models/` |

---

## 2. Folder layout

```
smartbin/
├── README.md                  ← this guide
├── server/                    ← runs on your computer
│   ├── app.py                 ← the detection server
│   ├── run_server.sh          ← start script (Linux / macOS)
│   ├── test_client.py         ← test the server with any image
│   ├── requirements.txt       ← Python packages
│   └── models/
│       └── smartbin_yolo11n_320.pt   ← ★ the AI model goes HERE
└── firmware/                  ← runs on the ESP32-CAM
    ├── platformio.ini
    ├── src/main.cpp
    └── include/
        ├── secrets.example.h  ← template
        └── secrets.h          ← you create this (Wi-Fi + server address)
```

> **Where is the model?**
> The server always loads **`server/models/smartbin_yolo11n_320.pt`**.
> Put your model file there with exactly that name.

---

## 3. Set up the server (your computer)

### 3.1 Unzip the project

Unzip `smartbin.zip` anywhere you like, for example your home folder or Desktop.
This guide calls that folder `smartbin/`.

### 3.2 Add the model

Copy your trained model into `server/models/` and name it `smartbin_yolo11n_320.pt`.

**Linux / macOS**
```bash
cp /path/to/your/model.pt smartbin/server/models/smartbin_yolo11n_320.pt
```

**Windows:** drag the file into `smartbin\server\models\` and rename it to `smartbin_yolo11n_320.pt`.

### 3.3 Create a Python environment and install packages

Open a terminal (Windows: **PowerShell**) **inside the `smartbin/server` folder**.

**Linux / macOS**
```bash
cd smartbin/server
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

**Windows (PowerShell)**
```powershell
cd smartbin\server
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

> **No GPU?** You can save about 2 GB of download by installing the CPU-only PyTorch **first**,
> before the line above:
> `pip install torch torchvision --index-url https://download.pytorch.org/whl/cpu`

This installs everything needed (Ultralytics YOLO, PyTorch, FastAPI, Uvicorn).
The first install takes a few minutes.

### 3.4 Start the server

**Linux / macOS**
```bash
chmod +x run_server.sh      # only needed the first time
./run_server.sh
```

The script prints the lines you will need for the ESP32. **Copy them.**

```text
------------------------------------------------------------
 Put this in firmware/include/secrets.h:
   #define SERVER_URL "http://192.168.1.50:8000/detect"
   #define API_KEY    "my-secret"
 Live view in a browser:  http://192.168.1.50:8000/view
------------------------------------------------------------
[smartbin] model loaded: models/smartbin_yolo11n_320.pt  classes={0: 'milk_packet', 1: 'yogurt_cup'}
```

**Windows (PowerShell)**
```powershell
$env:SMARTBIN_KEY = "my-secret"
uvicorn app:app --host 0.0.0.0 --port 8000
```
Then find your computer's IP address: run `ipconfig` and look for **IPv4 Address**
(for example `192.168.1.50`). Your server address is then:
`http://192.168.1.50:8000/detect`

> **Keep this terminal open.** The server runs only while this window is open.
> Press `Ctrl + C` to stop it.

### 3.5 Allow other devices to connect (firewall)

The ESP32 must be able to reach your computer on **port 8000**.

| System | What to do |
|---|---|
| **Ubuntu / Linux** | `sudo ufw allow 8000/tcp` |
| **Windows** | The first time you start the server, Windows asks to allow Python on the network. Click **Allow** (tick *Private networks*). |
| **macOS** | If asked "Accept incoming connections?", click **Allow**. |

---

## 4. Test the server without the ESP32

Do this **before** touching the ESP32. It proves the model and server work.

### 4.1 Send a test image

Open a **second terminal**, activate the environment again (step 3.3), then:

```bash
cd smartbin/server
python test_client.py path/to/any/photo.jpg
```

You should see something like:
```json
{
  "count": 1,
  "detections": [
    { "class": "milk_packet", "conf": 0.91, "box": [52, 40, 190, 210] }
  ],
  "ms": 24.3
}
```

### 4.2 Open the live view

In a browser on the same computer, open **http://localhost:8000/view**.
You should see your test image with a box drawn around the object.

### 4.3 Check other devices can reach it

On your **phone**, connected to the **same Wi-Fi**, open:

```
http://<your-computer-ip>:8000/
```

If you see `{"status":"ok", ...}`, the ESP32 will be able to connect too. ✅
If it doesn't load, see [Troubleshooting](#9-troubleshooting).

---

## 5. Set up the ESP32-CAM

### 5.1 Install PlatformIO (one time)

1. Open **VS Code**.
2. Click the **Extensions** icon (four squares) on the left.
3. Search **PlatformIO IDE** and click **Install**.
4. Restart VS Code when asked.

**Linux only:** allow your user to use USB serial ports, then **log out and log back in**:
```bash
sudo usermod -aG dialout $USER
```

**Windows only:** if the board doesn't show up as a COM port, install the USB driver for
your adapter (**CH340** for most ESP32-CAM-MB boards, or **CP210x**).

### 5.2 Open the firmware project

In VS Code: **File → Open Folder… → select `smartbin/firmware`**.

The first time, PlatformIO downloads the ESP32 tools. Wait until it finishes
(a few minutes; watch the bottom status bar).

### 5.3 Create your settings file

Copy the template:

| From | To |
|---|---|
| `firmware/include/secrets.example.h` | `firmware/include/secrets.h` |

Open `secrets.h` and fill in your details:

```cpp
#define WIFI_SSID  "YourWiFiName"        // 2.4 GHz network
#define WIFI_PASS  "YourWiFiPassword"

#define SERVER_URL "http://192.168.1.50:8000/detect"   // from step 3.4
#define API_KEY    "my-secret"                         // must match the server

#define USE_HTTPS  0                     // 0 = local computer
#define CAPTURE_INTERVAL_S 2             // seconds between photos
```

> `secrets.h` holds your Wi-Fi password. It is listed in `.gitignore`,
> so it is never uploaded to GitHub.

### 5.4 Connect the board

**Option A: ESP32-CAM-MB adapter (easiest)**
Press the ESP32-CAM onto the MB board and plug it in with USB.

**Option B: USB-TTL adapter**

| USB-TTL | ESP32-CAM |
|---|---|
| 5V | 5V |
| GND | GND |
| TX | U0R |
| RX | U0T |
| *(extra wire)* | **GPIO0 → GND** *(only while uploading)* |

### 5.5 Upload the firmware

Click the **→ (Upload)** arrow in the blue PlatformIO bar at the bottom of VS Code.

- Stuck on `Connecting........`?
  **MB board:** hold the **IO0** button, tap **RST**, then release IO0.
  **USB-TTL:** check the GPIO0 → GND wire is connected.
- **USB-TTL only:** after upload finishes, **remove the GPIO0 wire** and press **RST**.

### 5.6 Watch it start

Click the **🔌 (Serial Monitor)** icon in the bottom bar. You should see:

```text
=== Smart Bin ESP32-CAM ===
Connecting to YourWiFiName.....
Wi-Fi OK, ESP32 IP: 192.168.1.73
Server: http://192.168.1.50:8000/detect
[6123 bytes, 180 ms] {"count":1,"detections":[{"class":"milk_packet","conf":0.91,...}]}
```

🎉 **It works!** The ESP32 is now sending photos and getting detections back.

---

## 6. Use it

### 6.1 Three places to watch

| Where | What you see |
|---|---|
| **Server terminal** | One line per photo: `[detect] 240x240  25 ms -> milk_packet 0.91` |
| **Browser:** `http://<computer-ip>:8000/view` | Latest ESP32 photo with boxes, refreshes every second |
| **VS Code Serial Monitor** | What the ESP32 sent and the reply it got |

The small **red LED** on the back of the ESP32-CAM turns **on** when an object is detected.

### 6.2 Try it

Hold a milk packet, then a yoghurt cup, then nothing in front of the camera.
Try near, far, and in different lighting. Each result should appear within about a second.

### 6.3 Real photos are saved for you

Every photo the ESP32 sends is saved in **`server/debug/`**.
Use these to check the camera, or to label more training data (wrong detections,
missed objects and empty frames help most), then train a better model.

### 6.4 Make the bin do something

Open `firmware/src/main.cpp` and find these lines near the end:

```cpp
if (milk)   { /* TODO: open milk compartment (servo, relay...) */ }
if (yogurt) { /* TODO: open yoghurt compartment */ }
```

Put your servo, relay or buzzer code inside them, then upload again.

### 6.5 Update the model

1. Copy the new model into `server/models/smartbin_yolo11n_320.pt` (replace the old one).
2. Stop the server (`Ctrl + C`) and start it again.

The ESP32 does **not** need to be changed.

---

## 7. Starting it again next time

1. Open a terminal in `smartbin/server`.
2. Activate the environment:
   - Linux / macOS: `source .venv/bin/activate`
   - Windows: `.venv\Scripts\Activate.ps1`
3. Start the server:
   - Linux / macOS: `./run_server.sh`
   - Windows: `$env:SMARTBIN_KEY="my-secret"; uvicorn app:app --host 0.0.0.0 --port 8000`
4. Power on the ESP32-CAM. It reconnects automatically.
5. Open `http://<computer-ip>:8000/view` to watch.

> **Did your computer's IP address change?**
> Then update `SERVER_URL` in `secrets.h` and upload the firmware again.
> To stop this happening, set a **DHCP reservation** (fixed IP) for your computer in your router settings.

---

## 8. Settings you can change

### Server settings

Set these **before** starting the server.

| Setting | Default | What it does |
|---|---|---|
| `SMARTBIN_MODEL` | `models/smartbin_yolo11n_320.pt` | Path to the model file (`.pt` or `.onnx`) |
| `SMARTBIN_KEY` | `my-secret` | Password the ESP32 must send. Must match `API_KEY` in `secrets.h` |
| `SMARTBIN_CONF` | `0.5` | Minimum confidence (0–1). Higher = fewer false alarms |
| `SMARTBIN_IMGSZ` | `320` | Model input size. Must match the size the model was trained at |
| `SMARTBIN_SAVE` | `1` | `1` = save every photo to `server/debug/`, `0` = don't |
| `PORT` | `8000` | Server port (Linux/macOS script) |

**Example (Linux / macOS):**
```bash
SMARTBIN_CONF=0.7 SMARTBIN_KEY=myPassword123 ./run_server.sh
```

**Example (Windows PowerShell):**
```powershell
$env:SMARTBIN_CONF="0.7"; $env:SMARTBIN_KEY="myPassword123"
uvicorn app:app --host 0.0.0.0 --port 8000
```

### ESP32 settings (`firmware/include/secrets.h`)

| Setting | What it does |
|---|---|
| `WIFI_SSID`, `WIFI_PASS` | Your 2.4 GHz Wi-Fi |
| `SERVER_URL` | Your computer's address, ending in `/detect` |
| `API_KEY` | Must match `SMARTBIN_KEY` on the server |
| `USE_HTTPS` | `0` for your own computer; `1` only for an online (https) server |
| `CAPTURE_INTERVAL_S` | Seconds between photos |

After changing `secrets.h`, click **Upload** again.

### Tuning tips

| Problem | Try |
|---|---|
| Detects things that aren't there | Raise `SMARTBIN_CONF` to `0.6`–`0.7` |
| Misses real objects | Lower `SMARTBIN_CONF` to `0.35`–`0.4` |
| Too many photos | Raise `CAPTURE_INTERVAL_S` |
| Blurry or dark photos | Move the camera, add light, check `server/debug/` |
| Want more speed on CPU | Use the `.onnx` model: `pip install onnxruntime`, then set `SMARTBIN_MODEL=models/your_model.onnx` |

---

## 9. Troubleshooting

### Server problems

| What you see | Fix |
|---|---|
| `Model not found: models/smartbin_yolo11n_320.pt` | Put the model in `server/models/` with that exact name (step 3.2) |
| `uvicorn: command not found` | Activate the environment first (step 3.3) |
| `./run_server.sh: Permission denied` | Run `chmod +x run_server.sh` once |
| Phone can't open `http://<ip>:8000/` | Firewall (step 3.5); phone not on the same Wi-Fi; or the router has **AP / client isolation** on. Turn it off, or use a phone hotspot for both devices |

### ESP32 problems (shown in the Serial Monitor)

| What you see | Fix |
|---|---|
| `#error "Copy include/secrets.example.h ..."` | You haven't created `secrets.h` yet (step 5.3) |
| Wi-Fi dots never stop | Wrong password, or the network is 5 GHz only |
| `HTTP error -1` | ESP32 can't reach the server. Check the server is running, the IP in `SERVER_URL`, and the firewall. Do the phone test (step 4.3) |
| `HTTP error 401` | `API_KEY` in `secrets.h` ≠ `SMARTBIN_KEY` on the server |
| `HTTP error 400` | Camera sent a broken image. Check camera cable and power |
| `Camera init failed` | Reseat the camera ribbon cable (lift the black clip, push the cable in, close the clip) |
| Board keeps restarting / `Brownout detector` | Power too weak. Use a proper 5 V, 1 A+ supply |

### Upload problems (VS Code)

| What you see | Fix |
|---|---|
| Stuck on `Connecting......` | Put the board in flash mode: hold IO0, tap RST (MB), or connect GPIO0 → GND (USB-TTL) |
| `Permission denied: /dev/ttyUSB0` (Linux) | `sudo usermod -aG dialout $USER`, then log out and in |
| No COM port appears (Windows) | Install the CH340 or CP210x USB driver |
| Uploads fine but nothing happens | USB-TTL: remove the GPIO0 wire and press RST |

---

## How it works (short version)

1. The ESP32-CAM takes a **240 × 240 JPEG** photo.
2. It sends the photo to `POST /detect` on your computer, with the API key in the `X-API-Key` header.
3. The server runs **YOLO11n** (input size 320 × 320) on the photo.
4. It replies with JSON: the class (`milk_packet` / `yogurt_cup`), the confidence, and the box.
5. The ESP32 reads the reply, turns on its LED, and runs your bin action.

**Server endpoints**

| Endpoint | What it does |
|---|---|
| `GET /` | Health check: is the server running, which model is loaded |
| `POST /detect` | Send a JPEG, get detections back |
| `GET /latest.jpg` | The last photo with boxes drawn |
| `GET /view` | Web page that shows the latest photo live |
