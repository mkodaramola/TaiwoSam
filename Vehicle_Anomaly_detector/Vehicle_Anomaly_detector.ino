#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFiManager.h>  // AutoConnect + Captive Portal

// -------- Config --------
#define DEBUG 1
const char* serverURL = "http://your-firebase-url.com";  // Replace with your server URL

// -------- Sensor & GPS --------
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);  // RX=16, TX=17

// -------- Buffers --------
char gpsBuffer[128];
char payload[256];
char uploadStatus[] = "OK";

// -------- WiFiManager --------
WiFiManager wm;

// -------- Functions --------
bool connectToWiFi() {
  int CAPTIVE_PORTAL_TIMEOUT = 40;       // in seconds
  int AUTOCONNECT_WAIT_TIME = 20000;     // in ms

  WiFi.mode(WIFI_STA);
  WiFi.begin();  // Try to connect to known WiFi

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < AUTOCONNECT_WAIT_TIME) {
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }

  if (WiFi.status() == WL_CONNECTED) {
    #ifdef DEBUG
      Serial.println("\nAuto-connected to known WiFi.");
    #endif
    return true;
  }

  wm.setConfigPortalTimeout(CAPTIVE_PORTAL_TIMEOUT);
  #ifdef DEBUG
    Serial.println("\nAuto-connect failed. Starting captive portal...");
  #endif

  if (wm.autoConnect("SRAS_001", "12345678")) {
    #ifdef DEBUG
      Serial.println("Connected via captive portal.");
    #endif
    return true;
  } else {
    #ifdef DEBUG
      Serial.println("Captive portal timed out or user didn’t connect. Going offline.");
    #endif
    return false;
  }
}

void readGPS() {
  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
  }

  if (gps.location.isValid()) {
    snprintf(gpsBuffer, sizeof(gpsBuffer), "%.6f,%.6f,%.2f",
             gps.location.lat(),
             gps.location.lng(),
             gps.altitude.meters());
  } else {
    strcpy(gpsBuffer, "NA,NA,NA");
  }
}

void buildPayload() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accMag = sqrt(a.acceleration.x * a.acceleration.x +
                      a.acceleration.y * a.acceleration.y +
                      a.acceleration.z * a.acceleration.z);

  snprintf(payload, sizeof(payload),
           "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s",
           accMag, uploadStatus,
           a.acceleration.x, a.acceleration.y, a.acceleration.z,
           g.gyro.x, g.gyro.y, g.gyro.z,
           temp.temperature,
           gpsBuffer);

  #ifdef DEBUG
    Serial.print("Payload: ");
    Serial.println(payload);
  #endif
}

void sendData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data = "data=" + String(payload);
    int httpResponseCode = http.POST(data);

    #ifdef DEBUG
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    #endif

    http.end();
  } else {
    #ifdef DEBUG
      Serial.println("WiFi not connected. Skipping data upload.");
    #endif
  }
}

// -------- Setup --------
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif
  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17);  // RX, TX

  if (!mpu.begin()) {
    #ifdef DEBUG
      Serial.println("MPU6050 not detected. Check wiring.");
    #endif
    while (true) delay(10);
  }

  connectToWiFi();
}

// -------- Loop --------
void loop() {
  readGPS();
  buildPayload();
  sendData();
  delay(5000);  // adjust as needed
}