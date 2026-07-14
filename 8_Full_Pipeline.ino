#include "esp_camera.h"

#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"

#define BUTTON_PIN 21
#define DST_W      384
#define DST_H      512
#define CONTRAST   1.8

HardwareSerial printerSerial(1);

void resizeAndRotate(uint8_t* src, int srcW, int srcH,
                     uint8_t* dst, int dstW, int dstH) {
  float xScale = (float)srcH / dstW;
  float yScale = (float)srcW / dstH;
  for (int y = 0; y < dstH; y++) {
    for (int x = 0; x < dstW; x++) {
      float srcX = (dstW - 1 - x) * xScale;
      float srcY = y * yScale;
      int x0 = (int)srcX, y0 = (int)srcY;
      int x1 = min(x0+1, srcH-1), y1 = min(y0+1, srcW-1);
      float fx = srcX-x0, fy = srcY-y0;
      float val = src[y0*srcW+x0]*(1-fx)*(1-fy)
                + src[y0*srcW+x1]*fx*(1-fy)
                + src[y1*srcW+x0]*(1-fx)*fy
                + src[y1*srcW+x1]*fx*fy;
      dst[y*dstW+x] = (uint8_t)val;
    }
  }
}

void adjustContrast(uint8_t* img, int size, float factor) {
  for (int i = 0; i < size; i++) {
    float val = 128 + (img[i] - 128) * factor;
    val = max(0.0f, min(255.0f, val));
    img[i] = (uint8_t)val;
  }
}

void ditherAndPrint(uint8_t* img, int w, int h) {
  float* buf = (float*)ps_malloc(w * h * sizeof(float));
  if (!buf) { Serial.println("Dither buffer failed"); return; }
  for (int i = 0; i < w*h; i++) buf[i] = img[i];

  printerSerial.write(0x1B); printerSerial.write(0x40);
  delay(100);
  printerSerial.write(0x1D); printerSerial.write(0x76);
  printerSerial.write(0x30); printerSerial.write(0x00);
  printerSerial.write(w / 8);  printerSerial.write(0x00);
  printerSerial.write(h & 0xFF); printerSerial.write((h >> 8) & 0xFF);

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x += 8) {
      uint8_t byte = 0;
      for (int b = 0; b < 8; b++) {
        int idx = y * w + x + b;
        float oldVal = buf[idx];
        uint8_t newVal = oldVal < 128 ? 0 : 255;
        float err = oldVal - newVal;
        if (newVal == 0) byte |= (0x80 >> b);

        if (x+b+1 < w)
          buf[idx+1]   = constrain(buf[idx+1]   + err * 7/16.0, 0, 255);
        if (y+1 < h) {
          if (x+b-1 >= 0)
            buf[idx+w-1] = constrain(buf[idx+w-1] + err * 3/16.0, 0, 255);
          buf[idx+w]     = constrain(buf[idx+w]   + err * 5/16.0, 0, 255);
          if (x+b+1 < w)
            buf[idx+w+1] = constrain(buf[idx+w+1] + err * 1/16.0, 0, 255);
        }
      }
      printerSerial.write(byte);
      delayMicroseconds(1042);
    }
  }

  free(buf);
  printerSerial.write(0x0A);
  printerSerial.write(0x0A);
  Serial.println("Print complete!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  printerSerial.begin(9600, SERIAL_8N1, -1, 47);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED: 0x%x\n", err);
    return;
  }
  Serial.println("Ready — press button to capture and print!");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Capturing...");
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) { Serial.println("Capture failed"); delay(200); return; }

    uint8_t* resized = (uint8_t*)ps_malloc(DST_W * DST_H);
    if (!resized) {
      Serial.println("Resize buffer failed");
      esp_camera_fb_return(fb);
      delay(200);
      return;
    }

    resizeAndRotate(fb->buf, fb->width, fb->height, resized, DST_W, DST_H);
    esp_camera_fb_return(fb);
    adjustContrast(resized, DST_W * DST_H, CONTRAST);
    Serial.println("Processed — printing...");

    ditherAndPrint(resized, DST_W, DST_H);
    free(resized);
    delay(500);
  }
}
