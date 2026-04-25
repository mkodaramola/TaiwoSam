#include <SoftwareSerial.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>

#define DHTPIN 9

#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial BT(11,10);
LiquidCrystal_I2C lcd(0x27,16,2);

int buzzer = 7;

float turAv = 0;

volatile int flow_frequency; // Measures flow sensor pulses
unsigned int l_hour; // Calculated litres/hour
unsigned char flowsensor = 2; // Sensor Input
unsigned long currentTime;
unsigned long cloopTime;

byte c = 0;
int c1 = 0;

void flow () // Interrupt function
{
   flow_frequency++;
}

float otur = 0;
void setup() {
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));
  BT.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();

   pinMode(flowsensor, INPUT);
   digitalWrite(flowsensor, HIGH); // Optional Internal Pull-Up
   Serial.begin(9600);
   attachInterrupt(0, flow, RISING); // Setup Interrupt
   sei(); // Enable interrupts
   currentTime = millis();
   cloopTime = currentTime;

   pinMode(buzzer,OUTPUT);

   Mprint("Project by: ",0,0);
  Mprint("Wale & Moyin",1,0);
  delay(3000);
  lcd.clear();
  Mprint("Supervisor: ",0,0);
  Mprint("Dr. Ojediran",1,0);
  delay(3000);

}
float turbidity = 0;
void loop() {

  float mq2 = analogRead(A0);

   

  mq2 = (mq2*5.0)/134.0;



  currentTime = millis();
   // Every second, calculate and print litres/hour
   if(currentTime >= (cloopTime + 1000))
   {
      cloopTime = currentTime; // Updates cloopTime
      // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
      l_hour = (flow_frequency * 60 / 7.5); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
      flow_frequency = 0; // Reset Counter

    c++;

    turbidity += (float)analogRead(A1);

    Serial.print("TTT: "); Serial.println(turbidity);

    
    if (c > 4) {
      c1++;
      c = 0;

     turAv = turbidity/5;

     turAv = (turAv*2.5)/270.45;

     turbidity = 0;
      
    }

    if(c1 > 4) c1 = 0;
   }



  
 

  
  

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);


  Serial.println("\n");

  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));

  Serial.print(F("Heat index: "));
  Serial.print(hic);
  Serial.println(F("°C "));

  Serial.print("MQ: ");
  Serial.println(mq2);

  Serial.print("Turbidity: ");
  Serial.println(turAv);

  Serial.print("Water Flow Rate: ");
  Serial.println(l_hour);


   if (c1 == 0) {
    lcd.clear();
    Mprint("Temperature:",0,0);
    Mprint(String(t),0,12);
    Mprint("(degree Celsius)",1,0);
  }
  else if(c1 == 1){
      lcd.clear();
      Mprint("Heat Index: ",0,0);
    Mprint(String(hic),0,12);
    Mprint("(degree Celsius)",1,0);
    }
   else if(c1 == 2){
      lcd.clear();
      Mprint("Gas Level: ",0,0);
    Mprint(String(mq2),0,11);
    Mprint("(PPM)",1,0);
    }
  else if(c1 == 3){
      lcd.clear();
      Mprint("Turbidity: ",0,0);
    Mprint(String(turAv),0,11);
    Mprint("(ntu)",1,0);
    }
   else if(c1 == 4){
      lcd.clear();
      Mprint("Flow Rate: ",0,0);
    Mprint(String(l_hour),0,11);
    Mprint("(L/hour)",1,0);
    }

  if (mq2 > 9 || turAv > 15 || t > 40) digitalWrite(buzzer, HIGH);
  else digitalWrite(buzzer, LOW);
  
  

  BT.print(t);
  BT.print(",");
  BT.print(hic);
  BT.print(",");
  BT.print(mq2);
  BT.print(",");
  BT.print(turAv);
  BT.print(",");
  BT.print(l_hour);
  BT.print(";");

  
  delay(1000);
}


void Mprint(String tx, byte r,byte c){
      lcd.setCursor(c,r);
      lcd.print(tx);      
    }
