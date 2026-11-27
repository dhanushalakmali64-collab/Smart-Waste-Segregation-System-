"""
Smart Bin detection server.

The ESP32-CAM POSTs a raw JPEG to /detect. This server runs YOLO11n on it
and returns the detections as JSON.

Endpoints
  GET  /            health check            -> {"status": "ok", ...}
  POST /detect      raw JPEG body           -> {"count": N, "detections": [...]}
  GET  /latest.jpg  last frame with boxes   (open in a browser to watch)
  GET  /view        auto-refreshing page that shows /latest.jpg
"""

import os
import time
from datetime import datetime

import cv2
import numpy as np
from fastapi import FastAPI, Header, HTTPException, Request
from fastapi.responses import HTMLResponse, Response
from ultralytics import YOLO

# ---------------- settings (can be overridden with environment variables) ----
MODEL_PATH = os.environ.get("SMARTBIN_MODEL", "models/smartbin_yolo11n_320.pt")
API_KEY = os.environ.get("SMARTBIN_KEY", "my-secret")
IMGSZ = int(os.environ.get("SMARTBIN_IMGSZ", "320"))   # must match training size
CONF = float(os.environ.get("SMARTBIN_CONF", "0.5"))   # min confidence to report
SAVE_DEBUG = os.environ.get("SMARTBIN_SAVE", "1") == "1"  # save every received frame
DEBUG_DIR = "debug"
# ------------------------------------------------------------------------------

if not os.path.exists(MODEL_PATH):
    raise SystemExit(f"Model not found: {MODEL_PATH}  -> copy your .pt into server/models/")

model = YOLO(MODEL_PATH)
model.predict(np.zeros((IMGSZ, IMGSZ, 3), np.uint8), imgsz=IMGSZ, verbose=False)  # warm-up
print(f"[smartbin] model loaded: {MODEL_PATH}  classes={model.names}  imgsz={IMGSZ}")

os.makedirs(DEBUG_DIR, exist_ok=True)
app = FastAPI(title="Smart Bin detector")
latest_jpg: bytes | None = None


@app.get("/")
def health():
    return {"status": "ok", "model": os.path.basename(MODEL_PATH), "classes": model.names}


@app.post("/detect")
async def detect(request: Request, x_api_key: str = Header(None)):
    global latest_jpg
    if x_api_key != API_KEY:
        raise HTTPException(401, "bad API key")

    body = await request.body()
    img = cv2.imdecode(np.frombuffer(body, np.uint8), cv2.IMREAD_COLOR)  # BGR, as YOLO expects
    if img is None:
        raise HTTPException(400, "body is not a JPEG image")

    t0 = time.perf_counter()
    r = model.predict(img, imgsz=IMGSZ, conf=CONF, verbose=False)[0]
    ms = (time.perf_counter() - t0) * 1000

    dets = [
        {"class": r.names[int(c)], "conf": round(float(p), 3), "box": [round(v) for v in b]}
        for b, c, p in zip(r.boxes.xyxy.tolist(), r.boxes.cls.tolist(), r.boxes.conf.tolist())
    ]

    # keep the annotated frame for /latest.jpg and /view
    ok, buf = cv2.imencode(".jpg", r.plot())
    if ok:
        latest_jpg = buf.tobytes()

    # save the raw frame so you can inspect / re-label real ESP32 images later
    if SAVE_DEBUG:
        name = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        cv2.imwrite(f"{DEBUG_DIR}/{name}.jpg", img)

    summary = ", ".join(f"{d['class']} {d['conf']:.2f}" for d in dets) or "nothing"
    print(f"[detect] {img.shape[1]}x{img.shape[0]}  {ms:.0f} ms  -> {summary}")
    return {"count": len(dets), "detections": dets, "ms": round(ms, 1)}


@app.get("/latest.jpg")
def latest():
    if latest_jpg is None:
        raise HTTPException(404, "no frame received yet")
    return Response(latest_jpg, media_type="image/jpeg")


@app.get("/view", response_class=HTMLResponse)
def view():
    return """<!doctype html><html><head><title>Smart Bin live</title>
<style>body{background:#111;color:#eee;font-family:sans-serif;text-align:center}
img{width:480px;image-rendering:pixelated;border:1px solid #444;margin-top:12px}</style></head>
<body><h3>Smart Bin: latest ESP32 frame</h3><img id="f" alt="waiting for first frame...">
<script>setInterval(()=>{document.getElementById('f').src='/latest.jpg?'+Date.now()},1000)</script>
</body></html>"""
