#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F,16,4);
#define NOF 3

byte up = 13;
byte enter = 12;
byte down = 11;
byte back = 10;
byte page=0;

  byte chargePin = 9;
byte dischargePin=8;
boolean tst1 = true;
boolean tst2 = true;
unsigned long Btime=0;
unsigned long Etime=0;
double cap=0;
double Time=0;

String opt[NOF]={"< Voltmeter >","< Ohmeter >","< Cap-Meter >"};
byte y = 0;
byte bullet[8] = {

   0b11000,
 0b11100,
 0b11110,
 0b11111,
 0b11111,
 0b11110,
 0b11100,
 0b11000 
  };

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println("Multimeter");
  lcd.init();
  lcd.begin(16,2);
  lcd.createChar(1,bullet);
  lcd.backlight();
  lcd.setCursor(4,0);
  lcd.print("Multimeter");
    pinMode(up,INPUT);
  pinMode(enter,INPUT);
  pinMode(down,INPUT);
   pinMode(back,INPUT);
   pinMode(chargePin,OUTPUT);

  delay(1000); 
  lcd.clear();
  
  
}


void loop() {
  // put your main code here, to run repeatedly:
  if(page == 0) MenuPage();
  else if (page == 1) voltmeter();
   else if (page == 2) ohmeter();
    else if (page == 3) capMeter();
delay(20);
}

void MenuPage(){
lcd.clear(); 
lcd.setCursor(0,0);
lcd.write(1);
lcd.setCursor(1,0);
lcd.write(1);
lcd.setCursor(2,0);
lcd.write(1);
lcd.setCursor(3,0);
lcd.write(1);
lcd.setCursor(4,0);
lcd.write(1);
lcd.setCursor(2,1);
lcd.print(opt[y]);   

  if(digitalRead(up)== HIGH) {
  delay(700);
      Serial.println("Button Pressed");
    if(y == 0);
    else
      y--;
      
 
  }
  if(digitalRead(enter)== HIGH) {
    
    delay(700);
     Serial.println("Button Pressed");
      if(y == 0) page = 1;
      else if (y== 1) page = 2;
      else if (y==2) page = 3;
    }

   if(digitalRead(down)== HIGH) {
    delay(700);
        Serial.println("Button Pressed");
    if(y==(NOF-1));
    else 
      y++;
    }
  
  }
 void voltmeter(){
  if(digitalRead(enter)== HIGH) {
    
     delay(700);
          Serial.println("Button Pressed");
          lcd.clear();
      page = 0;
    }

  lcd.clear();
   float volt = analogRead(A1);
    volt = (volt*5)/470.5;

    volt = constrain(volt,0,9);
    
   lcd.setCursor(0,0);
    lcd.print("Voltage: ");
     lcd.setCursor(0,1);
    lcd.print(String(volt)+"v");
    Serial.println(volt);
    delay(20);

   
  
  
  } 


  
  String SRes="";
 void ohmeter(){

  
  if(digitalRead(enter)== HIGH) {
    
     delay(700);
          Serial.println("Button Pressed");
          lcd.clear();
      page = 0;
    }

 lcd.clear();
  float Res = analogRead(A2);
float Rvolt = (Res/1023)*5;

Res = (5000/Rvolt) - 1000; 
if(Rvolt == 0){
   SRes = "Insert A Resistor";
  }
else if(Res >= 1000){
Res = Res/1000;

SRes = String(Res);
SRes += "K";

}
else if(Res < 1000) {
  
  SRes = String(Res);
  }
  
lcd.setCursor(0,0);
lcd.print("Resistor: ");
lcd.setCursor(0,1);
lcd.print(SRes);

delay(20);

    
  }




 void capMeter(){

  //Serial.println(analogRead(A0));
  
  if(digitalRead(enter)== HIGH) {
    
     delay(700);
          Serial.println("Button Pressed");
          lcd.clear();
      page = 0;
    }

  lcd.clear();
digitalWrite(chargePin,HIGH);
  if(tst1 == true){
  Btime = millis();
  tst1 = false;
  }
  if(analogRead(A0)<375){

    
     if(tst2 == true){
    Etime = millis();
    tst2 = false;
    }
    
  Time = Etime - Btime;
   cap = (Time/10000*1000)+(0.07*(Time/10000*1000));
  
//    Serial.print("Btime: "); Serial.println(Btime);
//    Serial.print("Etime: "); Serial.println(Etime);
//    Serial.print("Time: "); Serial.println(Time);

    Serial.print("Capacitance: "); Serial.println(cap);
    lcd.setCursor(0,0);
lcd.print("Capacitance: ");
lcd.setCursor(0,1);
lcd.print(cap);

      digitalWrite(chargePin,LOW);
  pinMode(dischargePin,OUTPUT);
  digitalWrite(dischargePin,LOW);
  while(analogRead(A0)>0);
  pinMode(dischargePin,INPUT);
  
    tst1 = true;
    tst2 = true;
  delay(80);    
    }
  
  } 
