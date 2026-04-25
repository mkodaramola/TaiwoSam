#include <WiFi.h>
#include <ESP32Servo.h>
#include "DHT.h"

#define DHTPIN 27
#define MQ3PIN 33 // Define MQ3 sensor pin
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

Servo s1;
Servo s2;

// Network credentials here
const char* ssid     = "nano";
const char* password = "nanotest";

// Define the server host and port
const char* serverHost = "192.168.223.234"; // Replace with your server's IP address
const int serverPort = 80;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds
const long timeoutTime = 2000;

WiFiClient client;

bool grip = false;

void setup() {
  Serial.begin(115200);

  s1.attach(14); // Attach servo to GPIO 14
  s2.attach(26);

  dht.begin();

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Wi-Fi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Read DHT sensor values
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Compute heat index in Celsius
  float hic = dht.computeHeatIndex(t, h, false);

  // Read MQ3 sensor value
  int mq3Value = analogRead(MQ3PIN);
  String coL = "";
  if (mq3Value <= 100) coL = "Good";
  else if (mq3Value > 100 && mq3Value < 200) coL = "Fair";
  else coL = "HIGH";

  

  // Connect to the server
  if (client.connect(serverHost, serverPort)) {
    Serial.println("Connected to the server.");
    String gp = grip ? "Closed" : "Opened";
    // Send data to the server
    client.println("POST /update HTTP/1.1");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    String payload = createPayload(t, h, hic, coL, gp);
    client.println(payload.length());
    client.println();
    client.print(payload);

    // Wait for the response
    delay(1000);

    // Check if the server sent a response
    while (client.available()) {
      String response = client.readString();
      Serial.println("Response:");
      Serial.println(response);
      Serial.println("-----------------------");

      // Check for commands in the response
      if (response.indexOf("open") > 0) {
        grip = false;
      } else if (response.indexOf("close") > 0) {
        grip = true;
      }
    }
//192.168.150.234 - - [27/Nov/2024 07:17:11] "GET /set_gripper/close HTTP/1.1" 200 -

    client.stop();
  } else {
    Serial.println("Failed to connect to the server.");
  }

  // Adjust servo state based on the grip
  if (grip) _close();
  else _open();

  delay(5000); // Wait 5 seconds before sending the next update
}

String createPayload(float t, float h, float hic, String coL, String gp) {
  String payload = "{";
  payload += "\"temperature\":" + String(t) + ",";
  payload += "\"humidity\":" + String(h) + ",";
  payload += "\"heat_index\":" + String(hic) + ",";
  payload += "\"co_level\":\"" + coL + "\",";
  payload += "\"gripper_state\":\"" + gp + "\"";
  payload += "}";
  return payload;
}

void _close() {
  s1.write(80);
  s2.write(120);
}

void _open() {
  s1.write(170);
  s2.write(10);
}
