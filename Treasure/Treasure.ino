#include <WiFi.h>
#include <WiFiClient.h>


#define BLYNK_TEMPLATE_ID "TMPL264-xMGda"
#define BLYNK_TEMPLATE_NAME "IoT System"
#define BLYNK_AUTH_TOKEN "SHcDblprSfPhWebo_qGMYMN-3dePzWbJ"

#include <BlynkSimpleEsp32.h>  // Correct Blynk library for ESP32
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

const int trigPin = 5;
const int echoPin = 18;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials
char ssid[] = "Ayodele_Pump";
char pass[] = "Demilade";  


long duration;
float distanceCm;
float distanceInch;
bool Mstate = true;
String StrState = "OFF";

int fact = 15;
bool self = false;
int relay = 4;
int btn = 15;



BLYNK_WRITE(V2) {
  bool switchState = param.asInt(); 
  if (switchState == 1 && self == 1) {
    Mstate = true;
  } else if(switchState == 0 && self == 1) {
    Mstate = false;
  }
}

BLYNK_WRITE(V3) {
  bool auto_self = param.asInt(); // Read the value of V2 (0 or 1)
  if (auto_self == 1) {
    self = true;
  } else {
    self = false;
  }
}


void setup() {
  Serial.begin(115200); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(relay,OUTPUT);
  pinMode(btn,INPUT);
  lcd.init();
  lcd.backlight();

  Mprint("Project by: ",0,0);
  Mprint("T.M Ayodele",1,0);
  delay(2000);
  lcd.clear();

  Mprint("Supervisor: ",0,0);
  Mprint("Prof. Popoola",1,0);
  delay(2000);

  lcd.clear();

   WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Mprint("Waiting for",0,0);
    Mprint("Connection...",1,0);
  }
  Serial.println("Connected to WiFi");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.clear();

  
}

void loop() {

  Blynk.run();  // Run Blynk library

  if(!digitalRead(btn)){
      
      Serial.println("Calibrated!");
      fact = distanceCm;
      lcd.clear();
      Mprint("Calibrated!",0,2);
      delay(500);
      lcd.clear();
  }

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  Serial.println(distanceCm);
  int perc = (distanceCm/fact)*100;
  
  perc = constrain(perc, 0, 100);

  perc = 100-perc;

  // Prints the distance in the Serial Monitor
  Serial.print("Water Level: ");
  Serial.print(perc); Serial.println("%");

  lcd.clear();
  Mprint("Water Level:",0,0);
  Mprint(String(perc),0,13);
  if (perc > 9) Mprint("%",0,15);
  else Mprint("%",0,14);
  Mprint("Mach. State:",1,0);
  if(Mstate) {
        StrState = "ON";
        Serial.print ("Pumping Machine: "); Serial.println(StrState);
     }
    else {
        
        StrState = "OFF";
        Serial.print("Pumping Machine: "); Serial.println(StrState);
     }
  Mprint(StrState,1,13);


  Blynk.virtualWrite(V0, perc);      
  Blynk.virtualWrite(V1, StrState);     
 
  if (!self){
  if(perc > 80) Mstate = false;
  if(perc < 50) Mstate = true;
     }
  

  digitalWrite(relay,Mstate);
   
    

  delay(500);
}


void Mprint(String tx, byte r,byte c){
      lcd.setCursor(c,r);
      lcd.print(tx);      
    }
