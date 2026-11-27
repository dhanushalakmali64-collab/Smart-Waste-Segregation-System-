"""
Pretend to be the ESP32: send a JPEG to the server and print the answer.
Use this to check the server works BEFORE flashing the ESP32.

  python test_client.py path/to/image.jpg
  python test_client.py path/to/image.jpg --url http://192.168.1.50:8000/detect --key my-secret
"""

import argparse
import json
import urllib.request

import cv2

p = argparse.ArgumentParser()
p.add_argument("image")
p.add_argument("--url", default="http://127.0.0.1:8000/detect")
p.add_argument("--key", default="my-secret")
args = p.parse_args()

img = cv2.imread(args.image)
if img is None:
    raise SystemExit(f"cannot read {args.image}")
img = cv2.resize(img, (240, 240))  # same size the ESP32 sends
ok, buf = cv2.imencode(".jpg", img, [cv2.IMWRITE_JPEG_QUALITY, 85])

req = urllib.request.Request(
    args.url, data=buf.tobytes(), method="POST",
    headers={"Content-Type": "image/jpeg", "X-API-Key": args.key},
)
with urllib.request.urlopen(req, timeout=30) as resp:
    print(json.dumps(json.loads(resp.read()), indent=2))
