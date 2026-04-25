#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "Repent! Jesus loves you";   
const char* password = "Jesussaves.";

// Telegram bot token and chat ID
const String botToken = "7431523597:AAEYplwLRL_LDJ0Judm-UNNzHTooiC53H0g"; // Replace with your Telegram bot token
const String chatId = "5755481750";   

// MQ7 sensor pin
const int mq7Pin = 34; // Analog pin for MQ7 sensor
const int threshold = 100; // Set your CO level threshold here

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // Read CO level from MQ7 sensor
  int coLevel = analogRead(mq7Pin);
  coLevel = (coLevel*5.0)/20.0;
  Serial.print("CO Level: ");
  Serial.println(coLevel);

  // If CO level exceeds the threshold, send a notification to Telegram
  if (coLevel > threshold) {
    sendTelegramMessage(coLevel);
  }

  delay(5000); // Adjust as needed
}

void sendTelegramMessage(int coLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String message = "Warning! High CO level detected: " + String(coLevel);
    message.replace(" ", "%20"); // URL-encode the message (replace spaces with %20)
    
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + message;
    
    http.begin(url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Telegram message sent successfully.");
      Serial.println(response);
    } else {
      Serial.println("Error sending Telegram message");
    }
    
    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}
