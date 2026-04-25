#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define WIFI_SSID "Taiwosam"
#define WIFI_PASSWORD "taiwo1234"
#define FIREBASE_HOST "https://smartsystem-17bb5-default-rtdb.firebaseio.com" // Your Firebase URL

#define SOIL_MOISTURE_PIN 34
#define DHT_PIN 5
#define PUMP_PIN 4
#define DHT_TYPE DHT11

#define DRY_VALUE 3000  // Adjust based on sensor calibration
#define WET_VALUE 1000  // Adjust based on sensor calibration

byte THRESHOLD = 30; // Default threshold

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float humidity = 0;
float temperature = 0;

void Mprint(String tx, byte r, byte c) {
    lcd.setCursor(c, r);
    lcd.print(tx);
}

String removeQuotes(String input) {
    String result = "";
    for (int i = 0; i < input.length(); i++) {
        if (input[i] != '"' && input[i] != '\\') {
            result += input[i];
        }
    }
    return result;
}



void setup() {

    Serial.begin(9600);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lcd.init();
    lcd.backlight();

    Mprint("Project by: ", 0, 0);
    Mprint("S.A Taiwo", 1, 0);
    delay(2000);
    lcd.clear();
    Mprint("Supervisor: ", 0, 0);
    Mprint("Engr. Adesoba", 1, 0);
    delay(2000);
    lcd.clear();

    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);

    

    // Wait for WiFi connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        
        Mprint("Waiting for",0,0);
        Mprint("Connection....",1,0);
    }
    
    
    Serial.println("\nWiFi Connected!");

    dht.begin();

    THRESHOLD = getThresholdFromFirebase();  // Fetch initial threshold value
}

void loop() {
  Serial.println("----------- Begin");
    float soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);  
    float soilMoisture = map(soilMoistureRaw, WET_VALUE, DRY_VALUE, 100, 0);
    soilMoisture = constrain(soilMoisture, 0, 100);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

     if (isnan(h) || isnan(t)) {
    temperature = temperature;
    humidity = humidity;
    //return;
  }
  else {
    temperature = t;
    humidity = h;
  }

    Serial.print("Soil: "); Serial.print(soilMoisture); Serial.println("%");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.print("%  ");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println("°C");
    Serial.print("Threshold: "); Serial.println(THRESHOLD);

    lcd.clear();
    
    Mprint("Soil:" + String(soilMoisture) + "%", 1, 0);

    if (soilMoisture < THRESHOLD) {
        
        Mprint("Pump Mach.: ON", 0, 0);
        analogWrite(PUMP_PIN, 128);
      
    } else {
        analogWrite(PUMP_PIN, 0);
        Mprint("H:" + String(humidity) + "% T:" + String(temperature), 0, 0);
    }

    sendDataToFirebase(soilMoisture, humidity, temperature);
    THRESHOLD = getThresholdFromFirebase();

    Serial.println("----------- End");
  
    delay(5000); // Update every 5 seconds
    
}

void sendDataToFirebase(float soil, float humidity, float temperature) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(FIREBASE_HOST) + "/data.json"; // Send directly to the root
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{";
        jsonData += "\"soil\":" + String(soil) + ",";
        jsonData += "\"humidity\":" + String(humidity) + ",";
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

int getThresholdFromFirebase() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(FIREBASE_HOST) + "/threshold/threshold.json";  // Fetch only the threshold value
        http.begin(url);

        int httpResponseCode = http.GET();
        int newThreshold = THRESHOLD; // Default to the existing threshold if fetch fails

        if (httpResponseCode > 0) {
            String payload = http.getString();
            Serial.print("Threshold data received: ");
            Serial.println(payload);

            payload = removeQuotes(payload);

            Serial.println(payload);

            newThreshold = payload.toInt();  // Convert received value to integer
        } else {
            Serial.print("Failed to fetch threshold. Error: ");
            //Serial.println(http.errorToString(httpResponseCode).c_str());
        }

        http.end();
        return newThreshold;
    } else {
        Serial.println("WiFi not connected. Cannot fetch threshold from Firebase.");
        return THRESHOLD; // Return existing value if no WiFi
    }
}
