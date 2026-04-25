#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"   // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>             // Read and write from flash memory
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "sofolawe";
const char* password = "sofolawemichael";

// Telegram bot token and chat ID
const String botToken = "7431523597:AAEYplwLRL_LDJ0Judm-UNNzHTooiC53H0g"; // Replace with your Telegram bot token
const String chatId = "5755481750"; 

// Firebase credentials
String FIREBASE_HOST = "https://intruderalertdb-default-rtdb.firebaseio.com";
String FIREBASE_AUTH = "m6uuT7VcnBdOgiLZOPmaj249nEObNPXURPbbrI";
String firebaseImageKey = "/Intruder_Alert.json"; // Firebase key for image storage

const String websiteUrl = "https://dev-intrare.pantheonsite.io/";

// Define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

camera_config_t config;

void sendImagetoTelegram() {
  if (WiFi.status() == WL_CONNECTED) {
    // Turn on the flash LED
    digitalWrite(4, HIGH);

    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendPhoto";
    http.begin(url);
    http.addHeader("Content-Type", "multipart/form-data");

    String body = "--boundary\r\n";
    body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatId + "\r\n";
    body += "--boundary\r\n";
    body += "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";

    http.addHeader("Content-Length", String(body.length() + fb->len + 4)); // Additional bytes for end boundary
    http.addHeader("Content-Type", "multipart/form-data; boundary=boundary");

    int httpResponseCode = http.POST((uint8_t*)body.c_str(), body.length());
    http.addStream(fb->buf, fb->len);
    http.write((uint8_t*)"\r\n--boundary--\r\n", 15); // Close the boundary

    if (httpResponseCode > 0) {
      Serial.println("Image sent to Telegram successfully.");
    } else {
      Serial.println("Error in sending image to Telegram.");
    }

    http.end();
    esp_camera_fb_return(fb);

    // Turn off the flash LED
    digitalWrite(4, LOW);
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Camera configuration
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame size settings
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Set the flash LED as output
  pinMode(4, OUTPUT);

  // Send image to Telegram on reset
  sendImagetoTelegram();
}

void loop() {
  // Other code here
}
