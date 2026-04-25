#include <Wire.h>
#include "MAX30102_PulseOximeter.h"
#include <ESP8266WiFi.h>
#include <DHT.h>

// Define the DHT11 sensor
#define DHTPIN 12       // Pin where the DHT11 is connected
#define DHTTYPE DHT11   // DHT 11
#define REPORTING_PERIOD_MS 1000

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Replace with your network credentials
const char* ssid = "koolmind";      // Your Wi-Fi SSID
const char* password = "teamFUTA";  // Your Wi-Fi Password

WiFiServer server(80); // Create a server that listens on port 80

// PulseOximeter instance
PulseOximeter pox;
uint32_t tsLastReport = 0;

// Callback fired when a pulse is detected 
void onBeatDetected() {
    Serial.println("Beat!");
}

void setup() {
    Serial.begin(115200);
    dht.begin(); 
    
    // Initialize Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start the server
    server.begin();

    // Initialize the PulseOximeter instance
    Serial.print("Initializing MAX30102..");
    
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
    // Set the IR LED current
    pox.setIRLedCurrent(MAX30102_LED_CURR_7_6MA);
    // Register a callback for beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
    // Update sensor data
    pox.update();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); // Celsius
    
    // Fallback in case of sensor failure
    if (isnan(humidity) || isnan(temperature)) {
        humidity = -1;
        temperature = -1;
    }

    // Listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New Client connected.");

        // Wait until the client sends some data
        while (!client.available()) {
            delay(1);
        }

        // Read the first line of the request
        String request = client.readStringUntil('\r');
        Serial.println(request);
        client.flush();

        // Prepare JSON response
        String jsonResponse = "{";
        jsonResponse += "\"heartRate\":" + String(pox.getHeartRate()) + ",";
        jsonResponse += "\"spo2\":" + String(pox.getSpO2()) + ",";
        jsonResponse += "\"humidity\":" + String(humidity) + ",";
        jsonResponse += "\"temperature\":" + String(temperature);
        jsonResponse += "}";

        // HTTP response headers
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        // Send the JSON response
        client.println(jsonResponse);

        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
    }

    // Report heart rate and SpO2 to Serial Monitor every REPORTING_PERIOD_MS
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("Heart rate: ");
        Serial.print(pox.getHeartRate());
        Serial.print(" bpm / SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.print("% / Humidity: ");
        Serial.print(humidity);
        Serial.print("% / Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        tsLastReport = millis();
    }
}
