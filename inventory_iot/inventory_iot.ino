  #include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>

#endif

const char* ssid = "Repent! Jesus Loves You";   
const char* password = "Jesussaves.";

// Firebase URL and key
const String firebaseUrl = "https://smartsystem-17bb5-default-rtdb.firebaseio.com/"; // Firebase URL base
const String firebaseUnitsKey = "Units.json";

//pins:
const int HX711_dout = 27; //mcu > HX711 dout pin
const int HX711_sck = 14; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(0x27,16,2);

WiFiClient client;



const int calVal_eepromAdress = 0;
unsigned long t = 0;

float val_ = 0;
float w_u = 0;
boolean sw = true;
int units = 0;
const byte btn = 15;
unsigned long timer = 0;
unsigned long timer2 = 0;
float calibrationValue;
void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  WiFi.begin(ssid, password); 

  lcd.init();
  lcd.backlight();

  pinMode(btn,INPUT);

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
   // calibration value (see example file "Calibration.ino")
  calibrationValue = 1128.55; //696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete ");
    
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.clear();
    Mprint("Waiting for",0,0);
    Mprint("connection...",1,0);
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
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
 else units = round(val_/w_u);
  

  if (sw){
      if(millis() - timer >= 1000){
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
      Mprint("Units: ",0,0);
      Mprint(String(units),0,7);
      Serial.print("Units: "); Serial.println(String(units));

      // Send Units to Firebase
    if (sendFirebaseValue(units)) {
      Serial.println("Firebase update successful.");
    } else {
      Serial.println("Firebase update failed.");
    }
      
      timer = millis();

      
      }

      
    
      if (digitalRead(btn) == LOW){ 
        delay(1000);
        sw = false;
      }
    
    
    }

    else{


      if(millis() - timer2 >= 1000){
        lcd.clear();
       Mprint("Set Mass/Unit",0,0);
      
      timer2 = millis();
      }
      
      Serial.print("Mass/Unit: "); Serial.println(String(val_));
        
      if (digitalRead(btn) == LOW){     
        w_u = val_;
        delay(1000);
        sw = true;
      }
      
      
      }

    
 

  
  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }


}


void Mprint(String tx, byte r,byte c){
      
      lcd.setCursor(c,r);
      lcd.print(tx);      
    }


 bool sendFirebaseValue(int Units) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String jsonData = "{\"Units\":" + String(Units) + "}";

    http.begin(firebaseUrl + firebaseUnitsKey); // Specify the URL for mqvalue
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
