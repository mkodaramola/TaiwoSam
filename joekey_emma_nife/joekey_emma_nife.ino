#define BLYNK_TEMPLATE_ID "TMPL2pPI__Cv3"
#define BLYNK_TEMPLATE_NAME "Irrigation System"
#define BLYNK_AUTH_TOKEN "fxHJgSBoSP63GKHbKDxM0e2zzhQYEt1-"  

#define BLYNK_PRINT Serial  // Enable debug prints

#include <WiFi.h>
#include <WiFiClient.h>

#include <BlynkSimpleEsp32.h>  // Correct Blynk library for ESP32
#include <Wire.h>

#include "DHT.h"

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials
char ssid[] = "Beware";
char pass[] = "Gbenga10";  

// Define DHT sensor pin and type
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

int relay = 5;

unsigned long lastSentTime = 0;   // Tracks the last time data was sent
unsigned long interval = 2000;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Initialize Blynk

 

  pinMode(relay,OUTPUT);

  dht.begin();  // Initialize DHT sensor
}

void loop() {
  Blynk.run();  // Run Blynk library
  
  // Send sensor data every interval
  if (millis() - lastSentTime >= interval) {
    float moist = analogRead(35);
    moist = map(moist,0,4095,100,0);
    moist *= 2;
//    moist = abs(moist);
    int threshold = analogRead(34);
    threshold = map(threshold,0,4095,40,80);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      t = 0;
      return;
    }

    // Serial print of all variables
    Serial.print("Temperature: ");
    Serial.println(t);
    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("Soil Moisture: ");
    Serial.println(moist);
    Serial.print("Threshold: ");
    Serial.println(threshold);
    

    // Send data to Blynk
    Blynk.virtualWrite(V1, t);         // Send temperature to Virtual Pin V1
    Blynk.virtualWrite(V2, h);         // Send humidity to Virtual Pin V2
    Blynk.virtualWrite(V3, moist);     // Send moisture to Virtual Pin V3
    Blynk.virtualWrite(V4, threshold); // Send threshold to Virtual Pin V4

    Serial.println("Data sent to Blynk");

     if (t >= 16 && t <= 32 && moist < threshold)
        digitalWrite(relay,HIGH);
   else 
        digitalWrite(relay,LOW);

    lastSentTime = millis();  // Update the last sent time
  }

 
    
}




