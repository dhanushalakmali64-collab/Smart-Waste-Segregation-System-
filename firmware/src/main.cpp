// Smart Bin: ESP32-CAM client
// Captures a 240x240 JPEG, sends it to the detection server, prints the result.
// Board: AI-Thinker ESP32-CAM (OV2640)

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy include/secrets.example.h to include/secrets.h and fill in your Wi-Fi + server details"
#endif

// ---------- AI-Thinker ESP32-CAM camera pins ----------
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

#define STATUS_LED      33   // small red LED on the back, LOW = on

static bool initCamera() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;  c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;  c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = FRAMESIZE_240X240;   // same size as the training images
  c.jpeg_quality = 12;                  // 10-15: lower number = better quality, bigger file

  if (psramFound()) {
    c.fb_count    = 2;
    c.fb_location = CAMERA_FB_IN_PSRAM;
    c.grab_mode   = CAMERA_GRAB_LATEST; // always get a fresh frame
  } else {
    c.fb_count    = 1;
    c.fb_location = CAMERA_FB_IN_DRAM;
    c.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  return true;
}

static void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {           // give up after 20 s and retry later
      Serial.println("\nWi-Fi timeout, retrying...");
      WiFi.disconnect();
      return;
    }
  }
  Serial.printf("\nWi-Fi OK, ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

// Returns the server's JSON reply, or an empty string on failure.
static String sendFrame(camera_fb_t* fb) {
  HTTPClient http;
#if USE_HTTPS
  WiFiClientSecure client;
  client.setInsecure();                       // prototype only: skips certificate check
  http.begin(client, SERVER_URL);
  http.setTimeout(30000);                     // cloud cold start can be slow
#else
  http.begin(SERVER_URL);
  http.setTimeout(10000);
#endif
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-API-Key", API_KEY);

  int code = http.POST(fb->buf, fb->len);
  String body = (code == 200) ? http.getString() : "";
  if (code != 200) {
    Serial.printf("HTTP error %d %s\n", code, code < 0 ? http.errorToString(code).c_str() : "");
  }
  http.end();
  return body;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Smart Bin ESP32-CAM ===");
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);             // off

  if (!initCamera()) {
    Serial.println("Check the camera ribbon cable, then press Reset.");
    while (true) delay(1000);
  }
  connectWiFi();
  Serial.printf("Server: %s\n", SERVER_URL);
}

void loop() {
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) { delay(2000); return; }

  // with 1 frame buffer the first frame can be old: throw it away
  if (!psramFound()) {
    camera_fb_t* old = esp_camera_fb_get();
    if (old) esp_camera_fb_return(old);
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Capture failed");
    delay(1000);
    return;
  }

  uint32_t t0 = millis();
  size_t bytes = fb->len;
  String res = sendFrame(fb);
  esp_camera_fb_return(fb);

  if (res.length()) {
    Serial.printf("[%u bytes, %lu ms] %s\n", (unsigned)bytes, (unsigned long)(millis() - t0), res.c_str());

    bool milk   = res.indexOf("\"milk_packet\"") >= 0;
    bool yogurt = res.indexOf("\"yogurt_cup\"")  >= 0;
    digitalWrite(STATUS_LED, (milk || yogurt) ? LOW : HIGH);   // red LED on when something found

    if (milk)   { /* TODO: open milk compartment (servo, relay...) */ }
    if (yogurt) { /* TODO: open yoghurt compartment */ }
  }

  delay(CAPTURE_INTERVAL_S * 1000);
}
