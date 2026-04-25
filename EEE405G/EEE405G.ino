#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);

int pir = 9;
int buzzer = 6;
int relay = 13;

bool ef = false;
bool buzzOn = false;
long prevT = 0;

bool ef2 = true;
bool power = true;
long prevT2 = 0;

bool pirVal = 0;

bool ef3 = true;
long prevT3 = 0;

bool actM = false;
int ct = 0;
bool dbuzzer = false;
void setup() {
  // put your setup code here, to run once:
  

  Serial.begin(9600);
  pinMode(pir,INPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(relay,OUTPUT);


  lcd.init();               
  lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("COURSE: EEE 405");
  lcd.setCursor(0,1);
  lcd.print("PC: ENGR OLAIDE");
  delay(2000);

  lcd.clear();
  

}

void loop() {

int ldr = analogRead(A0); 

pirVal = digitalRead(pir);



if (ldr < 800) {
    actM = true;
    buzz();
  
  } else{
    ef = false;
    
    digitalWrite(buzzer,LOW);
    }


    if (actM && ct < 12){

      if(ef3) {

          prevT3 = millis();
          ef3 = false;
        
        }
      
      if (millis() - prevT3 >= 1000) {
        
        dbuzzer = !dbuzzer;
        ct++;
        prevT3 = millis();
        
      }

      digitalWrite(buzzer, dbuzzer);

      if (ct > 10) {
        ct = 0;
        actM = false; ef3 = true;
        }
      
      }

Serial.print("PIR - ");
Serial.println(pirVal);
Serial.print("LDR - ");
Serial.println(ldr);
powerOn();
   
delay(500);

}


void powerOn(){

   if (pirVal) {
    prevT2 = millis();

   }

     if(millis() - prevT2 >= 10000) power = LOW;
     else power = HIGH;

      digitalWrite(relay,power);


      lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Power: ");
     lcd.setCursor(7,0);
     if (power) lcd.print("ON");
     else lcd.print("OFF");
     

  }
  

  

  



void buzz(){
  
  if (!ef) {
    prevT = millis();

    ef = true;

  }
  

  

  if(millis() - prevT >= 1000){

    buzzOn = !buzzOn;

    

    prevT = millis();

  }

  digitalWrite(buzzer,buzzOn);

  if(buzzOn) {

     
     lcd.setCursor(0,1);
    lcd.print("Intruder Alert");
    Serial.println("Buzzer On");
    delay(500);
  
  }
  else{ 
    
    
   lcd.setCursor(0,1);
   lcd.print("");
    Serial.println("Buzzer Off");
  
  }


  
  }
   

  
  
  
  
