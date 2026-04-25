#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

// Replace with your network credentials
const char* ssid = "nano";   
const char* password = "nanotest";

// Firebase URL and key
const String firebaseUrl = "https://smartsystem-17bb5-default-rtdb.firebaseio.com/"; // Firebase URL base
const String firebaseMqValueKey = "mqvalue.json";
const String firebaseThresholdKey = "threshold.json";

// Telegram bot token and chat ID
const String botToken = "7340709572:AAGEG3SZOHVzDPVvQBx_WFeAAMBxwNCS5TI"; // Replace with your Telegram bot token
const String chatId = "716254952"; 

WiFiClient client;

LiquidCrystal_I2C lcd(0x27,16,2);

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

const int mq7 = 34;
int Thresh = 10;  // Default threshold value
const int buzzer = 27;
int mqAv;

String level = "LOW";

boolean ft = true;
int c = 0;
String extractNumber(String);
void sendTelegramMessage(int);
void getFirebaseThreshold();
bool sendFirebaseValue(int);
void Mprint(String , byte, byte);

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  WiFi.begin(ssid, password);  // Connect to Wi-Fi network

  lcd.init();
  lcd.backlight();
  
  Mprint("Project by: ",0,0);
  Mprint("M.S Sofolawe ",1,0);
  delay(3000);
  lcd.clear();
  Mprint("Supervisor: ",0,0);
  Mprint("Prof. Akingbade",1,0);
  delay(3000);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.clear();
    Mprint("Waiting for",0,0);
    Mprint("connection...",1,0);
  }
  Serial.println("Connected to WiFi");
  
  pinMode(buzzer, OUTPUT);
  
  lcd.clear();
  
  // Fetch the initial threshold value from Firebase
  getFirebaseThreshold();  
}

void loop() {
  // Check if the timer has elapsed
  int mqValue = 0;

  for (int i = 0; i < 200; i++) {
    mqValue += ((float)analogRead(mq7));
  }

  mqValue = mqValue / 200;
  mqValue = map(mqValue, 0, 1023, 0, 35);



  if ((millis() - lastTime) > timerDelay) {
    // Connect or reconnect to WiFi if not connected
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Attempting to reconnect to WiFi");
      WiFi.begin(ssid, password); 
      while (WiFi.status() != WL_CONNECTED) {
        delay(5000);     
      }
      Serial.println("\nReconnected to WiFi.");
    }

    // Send mqValue to Firebase
    if (sendFirebaseValue(mqValue)) {
      Serial.println("Firebase update successful.");
    } else {
      Serial.println("Firebase update failed.");
    }

    
    getFirebaseThreshold(); 

    c = 0;

      
    lastTime = millis();  // Update the timer
  }

  Serial.print("(");
  Serial.print(String(c));
  Serial.print(") ");
  Serial.println(mqValue);
  c++;
  
    lcd.clear();
    Mprint("CO Level:", 0, 0);
    Mprint(String(mqValue), 0, 10);
    Mprint("PPM", 0, 13);
  
    Mprint("Threshold:", 1, 0);
    Mprint(String(Thresh), 1, 11);
    
  if (mqValue >= Thresh) {
    digitalWrite(buzzer, HIGH);
    sendTelegramMessage(mqValue);
    level = "HIGH";
  } else if (mqValue > 5 && mqValue < Thresh) {
    level = "Fair";
    digitalWrite(buzzer, LOW);
  } else {
    digitalWrite(buzzer, LOW);
    level = "LOW";
  }

  delay(1000);
}

void Mprint(String tx, byte r, byte c) {
  lcd.setCursor(c, r);
  lcd.print(tx);      
}

// Function to send data to Firebase
bool sendFirebaseValue(int mqValue) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String jsonData = "{\"mqvalue\":" + String(mqValue) + "}";

    http.begin(firebaseUrl + firebaseMqValueKey); // Specify the URL for mqvalue
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    
    int httpResponseCode = http.PUT(jsonData); // Send PUT request
    
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

// Function to get the threshold value from Firebase
void getFirebaseThreshold() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(firebaseUrl + firebaseThresholdKey); // Specify the URL for threshold
    int httpResponseCode = http.GET(); // Send GET request
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Firebase threshold response: " + response);
      Serial.println("Firebase threshold response REFINED: " + extractNumber(response));
      // Parse the threshold value from the response and update Thresh
      String nr = response;
      nr = extractNumber(nr);
      Thresh = nr.toInt();
      Serial.println("Threshold value updated to: " + String(Thresh));
    } else {
      Serial.println("Error getting threshold from Firebase: " + String(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}

// Function to send a message via Telegram
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

//{"threshold":"\"23\""}
String extractNumber(String input) {
  String result = "";

  for (int i = 0; i < input.length(); i++) {
    // Check for opening and closing quotes
     if (isDigit(input[i])) {
      // If we're inside quotes and the character is a digit, add it to the result
      result += input[i];
    }
    else{
      
      continue;
      }
  }

  return result;  // Return the number found inside the quotes
}
