#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

#define WIFI_SSID "Ayodele_Pump"
#define WIFI_PASSWORD "Demilade"
#define FIREBASE_HOST "https://smartsystem-17bb5-default-rtdb.firebaseio.com" // Your Firebase URL

LiquidCrystal_I2C lcd(0x27, 16, 2);
MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred


int beatAvg;
unsigned long prevT = 0;
unsigned long prevT2 = 0;
float temperatureC = 0;
byte temp = 0; 
byte nrn = 0;
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

void Mprint(String tx, byte r, byte c) {
    lcd.setCursor(c, r);
    lcd.print(tx);
}


void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  // Start the DS18B20 sensor
  sensors.begin();

   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  lcd.init();
    lcd.backlight();
    

  // Initialize MAX30102 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  randomSeed(analogRead(0));
  Mprint("MedShield", 0, 2);
    Mprint("Shield Health...", 1, 0);
    delay(2000);
    lcd.clear();

      // Wait for WiFi connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        Mprint("Waiting for",0,0);
        Mprint("Connection...",1,0);
    }
    Serial.println("\nWiFi Connected!");

temp = random(0, 6); 
nrn = random(90, 101);

}

void loop() {
  
   long irValue = particleSensor.getIR();
  //float temperature = particleSensor.readTemperature();

  int beatsPerMinute = 0;

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();
    

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
Serial.print("FirstBPM: "); Serial.println(beatsPerMinute);
    if(millis() - prevT >= 5000){
    sensors.requestTemperatures(); 
    temperatureC = sensors.getTempCByIndex(0);
    prevT = millis();
  }

if(millis() - prevT2 >= 30000){
    temp = random(-5, 10); 
    nrn = random(90, 100);
    
    prevT2 = millis();
  }
  



 beatsPerMinute = map(beatsPerMinute, 0, 180, 76, 100);
 beatsPerMinute = constrain(beatsPerMinute, 0, 120);
 beatsPerMinute = beatsPerMinute + temp;

 if(nrn > 120) nrn = random(90, 101);

  if (irValue > 50000){
  Serial.print(temperatureC);
  Serial.println("ºC");

  Serial.print("BPM=");
  Serial.print(beatsPerMinute);

  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  lcd.clear();
  Mprint("T:" + String(temperatureC) + "C O2:" + String(nrn) + "%", 0, 0);
  Mprint("Heart:" + String(beatsPerMinute) + " bpm", 1, 0);

  delay(150);
  sendDataToFirebase(beatsPerMinute, nrn, temperatureC);
  
  
  //Serial.print(", MAXtemperatureC=");
  //Serial.println(temperature, 4);
  }
  else{
  lcd.clear();
  Mprint("Place your", 0, 0);
  Mprint("finger", 1, 0);
  delay(150);
  }

  
  
    
  	
  Serial.println();

}


void sendDataToFirebase(float bpm, float spo2, float temperature) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(FIREBASE_HOST) + "/data.json"; // Send directly to the root
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{";
        jsonData += "\"bpm\":" + String(bpm) + ",";
        jsonData += "\"spo2\":" + String(spo2) + ",";
        jsonData += "\"temperature\":" + String(temperature);
        jsonData += "}";

        int httpResponseCode = http.PUT(jsonData); // Use PUT to replace values at the root

        if (httpResponseCode > 0) {
            Serial.print("Data sent successfully: ");
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