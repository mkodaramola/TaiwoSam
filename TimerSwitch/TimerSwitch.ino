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
boolean Tinit=true;
unsigned long Stime=0;
unsigned long Utime=0;
int Tspent=0;
int Trem = 0;
byte buzzer = 4;
void setup() {
  // put your setup code here, to run once:

  pinMode(relay,OUTPUT);
  pinMode(buzzer,OUTPUT);
  DDRB = B00000000;
  
Serial.begin(9600);
  lcd.init();               
  lcd.begin(16,2);
  lcd.backlight();
  lcd.createChar(1,pointer);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("|- Eltronics -|");
  lcd.setCursor(1,1);
  lcd.print("Timer Switch");
  delay(2000);

  lcd.clear();
}

void loop() {
  // put your main code here, to run repeatedly:
 
 if (page == 1)
  select();
 if (page == 2)
  start();

}

void select(){
   
  digitalWrite(relay,LOW);
  
  lcd.setCursor(x,0);
  lcd.write(1);
  lcd.setCursor(8,0);
  lcd.print("<- Set");
  
  lcd.setCursor(11,1);
  lcd.print("Time");
  
  pMin = setTime.indexOf(":");
  pMin+=1;
  pSec = setTime.lastIndexOf(":");
  pSec+=1;
  //Right
  if(digitalRead(right) == HIGH){
   delay(100);

    if(x == 0) x = pMin;
    else if(x == pMin) x = pSec;
    }

  //Left
      if(digitalRead(left) == HIGH){
    delay(100);
    if(x == pSec) x = pMin;

    else if(x == pMin) x = 0;
    }

  //up
    if(digitalRead(up) == HIGH){
     delay(100);
    if(x == 0) {
      
      if(hr <= 0) hr=0;
      else
        hr--;

      }

      if(x == (setTime.indexOf(":"))+1) {
      
      if(mins <= 0) mins=59;
      else
        mins--;
      }

      if(x == (setTime.lastIndexOf(":"))+1) {
      
      if(sec <= 0) sec=59;
      else
        sec--;
      }
    
    }

  //down

   if(digitalRead(down) == HIGH){
    delay(100);
    if(x == 0) {
      
      if(hr >= 99) hr=99;
      else
        hr++;

      }

      if(x == (setTime.indexOf(":"))+1) {
      
      if(mins >=59) mins=0;
      else
        mins++;
      }

      if(x == (setTime.lastIndexOf(":")+1)) {
      
      if(sec >= 59) sec=0;
      else
        sec++;
      }
    
    }
setTime = String(hr)+":"+String(mins)+":"+String(sec);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(setTime);

  if(digitalRead(enter) == HIGH){
      delay(100);
      Utime = (hr*3600)+(mins*60)+sec;
      page = 2;
      Serial.print("User Time: "); Serial.println(Utime);
    }
  
  }

byte Rhr;
byte Rmin;
byte Rsec; 

  void start(){
    
    

    if(Tinit == true){
      Stime = millis();
      Tinit = false;
     Serial.print("STime: "); Serial.println(Stime);
      }

 digitalWrite(buzzer,LOW);
    while((millis() - Stime <= (Utime*1000)) && digitalRead(back) == LOW){
      digitalWrite(relay,HIGH);
      Tspent = (millis() - Stime)/1000;
      Trem = Utime - Tspent;
      
      Rhr = Trem/3600;
      Rmin = (Trem%3600)/60;
      Rsec = (Trem%3600) - (Rmin*60);
      lcd.setCursor(0,0);
      lcd.print("Time left:");
      lcd.clear();
      lcd.setCursor(0,1);
      lcd.print(String(Rhr)+":"+String(Rmin)+":"+String(Rsec));

      Serial.print("millis() "); Serial.println(millis());

      if(digitalRead(back) == HIGH){
      delay(100);
      page = 1;
      Tinit = true;
    }
      
      }

      if(millis() - Stime >= (Utime*1000)){
            lcd.clear();
            digitalWrite(relay,LOW);
            lcd.setCursor(3,0);
            lcd.print("Done!");
            buzzerAlert();

            
        }



    
    if(digitalRead(back) == HIGH){
      delay(100);
      page = 1;
      Tinit = true;
    }

    }

boolean buzzOn = false;
int prevT=0;
void buzzerAlert(){
      
      if(millis()-prevT >= 500) {
          buzzOn = !buzzOn;
          prevT = millis();
        } 

     digitalWrite(buzzer,buzzOn);
  }

  
