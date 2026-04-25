#include <ThingSpeak.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

// Replace with your network credentials
const char* ssid = "Repent! Jesus loves you";   
const char* password = "Jesussaves.";

// Telegram bot token and chat ID
const String botToken = "7431523597:AAEYplwLRL_LDJ0Judm-UNNzHTooiC53H0g"; // Replace with your Telegram bot token
const String chatId = "5755481750"; 

WiFiClient client;

LiquidCrystal_I2C lcd(0x27,16,2);

unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "13EG5M7EAZRGNOTU";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

const int mq7 = 34;
const int Thresh = 10;
const int buzzer = 27;
int mqAv;

String level = "LOW";

boolean ft = true;
int c = 0;
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

  
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  pinMode(buzzer,OUTPUT);
  
  
  lcd.clear();
}

void loop() {
  // Check if the timer has elapsed
  int mqValue = 0;

  

 
    for(int i=0; i<200; i++){
        mqValue += ((float)analogRead(mq7));
    }

    mqValue = mqValue/200;

    mqValue = map(mqValue,0,1023,0,100);

    if (ft){

    lcd.clear();
    Mprint("CO Level:",0,0);
    Mprint(String(mqValue),0,10);
    Mprint("PPM",0,13);
  
    Mprint("Status:",1,0);
    Mprint(level,1,8);
    ft = false;
        
    }
  
  if ((millis() - lastTime) > timerDelay) { 
    
    // Connect or reconnect to WiFi if not connected
    if(WiFi.status() != WL_CONNECTED) {
      Serial.print("Attempting to connect");
      WiFi.begin(ssid, password); 
      while(WiFi.status() != WL_CONNECTED) {
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    // Write to ThingSpeak
    int x = ThingSpeak.writeField(myChannelNumber, 1, mqValue, myWriteAPIKey);
    Serial.println(mqValue);

    lcd.clear();
     Mprint("CO Level:",0,0);
    Mprint(String(mqValue),0,10);
    Mprint("PPM",0,13);
  
    Mprint("Status:",1,0);
    Mprint(level,1,8);


    if(x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }

    c = 0;
    lastTime = millis();  // Update the timer
  }
  Serial.print("(");
  Serial.print(String(c));
  Serial.print(") ");
  Serial.println(mqValue);
  c++;
  
  if(mqValue >= Thresh){
      digitalWrite(buzzer,HIGH);
      sendTelegramMessage(mqValue);
      level = "HIGH";
    }
    else if (mqValue > 5 && mqValue < Thresh){
        level= "Fair";
        digitalWrite(buzzer,LOW);
      }
   else {
        digitalWrite(buzzer,LOW);
        level = "LOW";
    }
  
  
  
  delay(1000);
}

void Mprint(String tx, byte r,byte c){
      lcd.setCursor(c,r);
      lcd.print(tx);      
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
