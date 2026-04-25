#include <ESP8266WiFi.h>           // Change WiFi library to ESP8266WiFi
#include <ESP8266HTTPClient.h>     // Change HTTPClient library to ESP8266HTTPClient
#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#include <EEPROM.h>                // EEPROM library works on ESP8266 as well

const char* ssid = "nano";   
const char* password = "nanotest";

// Firebase URL and key
const String firebaseUrl = "https://smartsystem-17bb5-default-rtdb.firebaseio.com/";
const String firebaseUnitsKey = "Units.json";

// Pins (updated for ESP8266 GPIO layout)
const int HX711_dout = 12;    // ESP8266 GPIO pin for HX711 dout
const int HX711_sck = 14;     // ESP8266 GPIO pin for HX711 sck

// HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient client;

const int calVal_eepromAddress = 0;
unsigned long t = 0;

float val_ = 0;
float w_u = 0;
boolean sw = true;
int units = 0;
const byte btn = 13;          // ESP8266 GPIO pin for button
unsigned long timer = 0;
unsigned long timer2 = 0;
float calibrationValue;

void setup() {
  Serial.begin(57600);
  Serial.println("\nStarting...");

  WiFi.begin(ssid, password);

  lcd.init();
  lcd.backlight();

  pinMode(btn, INPUT);

  LoadCell.begin();
  calibrationValue = 1128.55;

  EEPROM.begin(512);                        // EEPROM initialization for ESP8266
  EEPROM.get(calVal_eepromAddress, calibrationValue);

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    //while (1);
  } else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.clear();
    Mprint("Waiting for", 0, 0);
    Mprint("connection...", 1, 0);
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      val_ = LoadCell.getData();
      val_ = val_ - 50;
      Serial.println(calibrationValue);
      Serial.print("Weight: ");
      Serial.println(val_);
      newDataReady = 0;
      t = millis();
    }
  }

  if (w_u == 0) units = 0;
  else units = round(val_ / w_u);

  if (sw) {
    if (millis() - timer >= 1000) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Attempting to reconnect to WiFi");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          delay(5000);
        }
        Serial.println("\nReconnected to WiFi.");
      }

      if (units < 0) units = 0;

      lcd.clear();
      Mprint("Units: ", 0, 0);
      Mprint(String(units), 0, 7);
      Serial.print("Units: ");
      Serial.println(String(units));

      // Send Units to Firebase
      if (sendFirebaseValue(units)) {
        Serial.println("Firebase update successful.");
      } else {
        Serial.println("Firebase update failed.");
      }

      timer = millis();
    }

    if (digitalRead(btn) == LOW) {
      delay(1000);
      sw = false;
    }
  } else {
    if (millis() - timer2 >= 1000) {
      lcd.clear();
      Mprint("Set Mass/Unit", 0, 0);
      timer2 = millis();
    }

    Serial.print("Mass/Unit: ");
    Serial.println(String(val_));

    if (digitalRead(btn) == LOW) {
      w_u = val_;
      delay(1000);
      sw = true;
    }
  }

  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}

void Mprint(String tx, byte r, byte c) {
  lcd.setCursor(c, r);
  lcd.print(tx);
}

bool sendFirebaseValue(int Units) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String jsonData = "{\"Units\":" + String(Units) + "}";

    http.begin(client, firebaseUrl + firebaseUnitsKey); // Include client for ESP8266
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.PUT(jsonData);

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
