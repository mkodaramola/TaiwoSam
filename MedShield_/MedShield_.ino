#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// WiFi Credentials
#define WIFI_SSID "Ayodele_Pump"
#define WIFI_PASSWORD "Demilade"
#define FIREBASE_HOST "https://smartsystem-17bb5-default-rtdb.firebaseio.com"
#define BUFFER_SIZE 100

// LCD Display Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// MAX30105 Sensor Setup
MAX30105 particleSensor;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

// Buffer for Sensor Data
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Sensor Readings
int32_t spo2;
int8_t spo2Valid;
int32_t heartRate;
int8_t hrValid;
int beatAvg;
unsigned long prevT = 0;
float temperatureC = 0;

// DS18B20 Temperature Sensor Setup
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void Mprint(String message, byte row, byte col) {
    lcd.setCursor(col, row);
    lcd.print(message);
}

void setup() {
    Serial.begin(115200);
    sensors.begin();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lcd.init();
    lcd.backlight();
    
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30105 was not found. Please check wiring/power.");
        while (1);
    }

    Serial.println("Place your index finger on the sensor with steady pressure.");
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    randomSeed(analogRead(0));

    Mprint("MedShield", 0, 2);
    Mprint("Shield Health...", 1, 0);
    delay(2000);
    lcd.clear();

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        Mprint("Waiting for", 0, 0);
        Mprint("Connection...", 1, 0);
    }
    Serial.println("\nWiFi Connected!");
}

void loop() {
    long irValue = particleSensor.getIR();
    int beatsPerMinute = 0;

    if (checkForBeat(irValue)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();
        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;
            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }

    if (millis() - prevT >= 5000) {
        sensors.requestTemperatures();
        temperatureC = sensors.getTempCByIndex(0);
        prevT = millis();
    }

    irBuffer[bufferIndex] = particleSensor.getIR();
    redBuffer[bufferIndex] = particleSensor.getRed();
    bufferIndex++;

    if (bufferIndex >= BUFFER_SIZE) {
        maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &spo2Valid, &heartRate, &hrValid);
        if (spo2Valid && spo2 > 0 && spo2 <= 100) {
            Serial.print("SpO2: ");
            Serial.print(spo2);
            Serial.println("%");
        } else {
            Serial.println("Invalid SpO2 reading");
        }
        bufferIndex = 0;
    }

    if (irValue > 50000) {
        Serial.print(temperatureC);
        Serial.println("ºC");
        Serial.print("BPM=");
        Serial.println(beatsPerMinute);

        lcd.clear();
        Mprint("T:" + String(temperatureC) + "C O2:" + String(spo2) + "%", 0, 0);
        Mprint("Heart:" + String(beatsPerMinute) + " bpm", 1, 0);
        delay(150);

        sendDataToFirebase(beatsPerMinute, spo2, temperatureC);
    } else {
        lcd.clear();
        Mprint("Place your", 0, 0);
        Mprint("finger", 1, 0);
        delay(150);
    }
}

void sendDataToFirebase(float bpm, float spo2, float temperature) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(FIREBASE_HOST) + "/data.json";
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{";
        jsonData += "\"bpm\":" + String(bpm) + ",";
        jsonData += "\"spo2\":" + String(spo2) + ",";
        jsonData += "\"temperature\":" + String(temperature);
        jsonData += "}";

        int httpResponseCode = http.PUT(jsonData);

        if (httpResponseCode > 0) {
            Serial.println("Data Sent Successfully");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("Failed to send data. Error: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }

        http.end();
    } else {
        Serial.println("WiFi not connected. Cannot send data to Firebase.");
    }
}
