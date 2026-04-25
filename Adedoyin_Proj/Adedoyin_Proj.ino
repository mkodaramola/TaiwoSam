#include "Base64.h"  
#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"   // Disable brownout problems
#include "WiFi.h"               // WiFi library for ESP32
#include <base64.h>
#include "HTTPClient.h"         // For HTTP requests
           // Base64 encoding library

// WiFi credentials
const char* ssid = "Repent! Jesus Loves You";
const char* password = "Jesussaves.";

// Firebase credentials
String FIREBASE_HOST = "https://intruderalertdb-default-rtdb.firebaseio.com";
String FIREBASE_AUTH = "m6uuT7VdVcnBdOgiLZOPmaj249nEObNPXURPbbrI";
String firebaseMqValueKey = "/images.json"; // Firebase key for image storage


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

String urlencode(String str) {
  const char *msg = str.c_str();
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~') {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[(unsigned char)*msg >> 4];
      encodedMsg += hex[*msg & 0xf];
    }
    msg++;
  }
  return encodedMsg;
}

bool sendFirebaseValue(String image_b64) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String firebaseUrl = FIREBASE_HOST + firebaseMqValueKey + "?auth=" + FIREBASE_AUTH;
    String jsonData = "{\"SavedImage\":\"" + image_b64 + "\"}";

    http.begin(firebaseUrl); // Specify the URL for Firebase
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    
    int httpResponseCode = http.PATCH(jsonData); // Send PATCH request (use PATCH instead of PUT)
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Firebase response: " + response);
      http.end();
      return true;
    } else {
      Serial.println("Error sending data to Firebase: " + String(httpResponseCode));
      http.end();
      return false;
    }
  } else {
    Serial.println("Error: Not connected to WiFi");
    return false;
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);

  // Start WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  pinMode(13, INPUT);  // Set GPIO 13 as input for the button

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // UXGA for higher quality
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // Lower resolution
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }
}

void loop() {
  // Check if button is pressed (assume GPIO 13 is connected to the button)
  if (digitalRead(13) == HIGH) {
    // Capture Image from Camera
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Allocate memory for the base64-encoded image
    int base64Length = base64_enc_len(fb->len);  // Calculate the length of the encoded string
    char *image_b64 = (char *)malloc(base64Length + 1);  // +1 for null terminator

    // Perform base64 encoding
    base64_encode(image_b64, (char*)fb->buf, fb->len);

    // Release the frame buffer
    esp_camera_fb_return(fb);

    // Send the image to Firebase
    if (sendFirebaseValue(image_b64)) {
      Serial.println("Image sent to Firebase successfully");
    } else {
      Serial.println("Failed to send image to Firebase");
    }

    // Free the allocated memory for the base64 string
    free(image_b64);

    delay(5000); // Wait before the next picture
  }
}
