#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);

byte pointer[8] = {
  
 0b0000,
 0b00000,
 0b00000,
 0b00000,
 0b11111,
 0b01110,
 0b00100  
  
  };

byte hr=0;
byte mins=0;
byte sec=0;
byte relay = 3;
byte up = 10;
byte down = 11;
byte left = 9;
byte right = 12;
byte back = 13;
byte enter = 8;
String setTime = String(hr)+":"+String(mins)+":"+String(sec);
byte x=0;
byte pMin=0;
byte pSec=0; 
byte page = 1;
void setup() {
  // put your setup code here, to run once:

  pinMode(relay,OUTPUT);
  pinMode(up,INPUT);
  pinMode(down,INPUT);
  pinMode(right,INPUT);
  pinMode(left,INPUT);
  pinMode(back,INPUT);
  pinMode(enter,INPUT);
  
Serial.begin(9600);
  lcd.init();               
  lcd.begin(16,2);
  lcd.backlight();
  lcd.createChar(1,pointer);
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("|Eltronics|");
  lcd.setCursor(3,1);
  lcd.print("Timer Switch");
  delay(2000);

  lcd.clear();
}

void loop() {
  // put your main code here, to run repeatedly:
 lcd.clear();
 if (page == 1)
  select();
 if (page == 2)
  start();

}

void select(){
  
  digitalWrite(relay,LOW);
  lcd.setCursor(x,0);
  lcd.write(1);
  pMin = setTime.indexOf(":");
  pMin+=1;
  pSec = setTime.lastIndexOf(":");
  pSec+=1;
  //Right
  if(digitalRead(right) == HIGH){
   delay(100);

    if(x == 0) x = pMin;
    else if(x == pMin) x = pSec;

     Serial.print("x: "); Serial.println(x);
    }

  //Left
      if(digitalRead(left) == HIGH){
    delay(100);
    if(x == pSec) x = pMin;

    else if(x == pMin) x = 0;

     Serial.print("x: "); Serial.println(x);
    }

  //up
    if(digitalRead(up) == HIGH){
     delay(100);
    if(x == 0) {
      
      if(hr <= 0) hr=0;
      else
        hr--;

        Serial.print("ux: "); Serial.println(x);
      }

      if(x == (setTime.indexOf(":"))+1) {
      
      if(mins <= 0) mins=59;
      else
        mins--;

        Serial.print("ux: "); Serial.println(x);
      }

      if(x == (setTime.lastIndexOf(":"))+1) {
      
      if(sec <= 0) sec=59;
      else
        sec--;

        Serial.print("ux: "); Serial.println(x);
      }
    
    }

  //down

   if(digitalRead(down) == HIGH){
    delay(100);
    if(x == 0) {
      
      if(hr >= 99) hr=99;
      else
        hr++;

        Serial.print("dx: "); Serial.println(x);
      }

      if(x == (setTime.indexOf(":"))+1) {
      
      if(mins >=59) mins=0;
      else
        mins++;

        Serial.print("dx: "); Serial.println(x);
      }

      if(x == (setTime.lastIndexOf(":")+1)) {
      
      if(sec >= 59) sec=0;
      else
        sec++;

        Serial.print("dx: "); Serial.println(x);
      }
    
    }
setTime = String(hr)+":"+String(mins)+":"+String(sec);
  lcd.setCursor(0,1);
  lcd.print(setTime);

  if(digitalRead(enter) == HIGH){
      delay(100);
      page = 2;
    }
  
  }


  void start(){
    
    digitalWrite(relay,HIGH);
    if(digitalRead(back) == HIGH){
      delay(100);
      page = 1;
    }
    }
