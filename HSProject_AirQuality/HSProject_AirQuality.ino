//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

int buzzer = 11;


void setup(){
  lcd.init();                      // initialize the lcd 

  Serial.begin(9600);
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("Hello, world!");

  pinMode(buzzer, OUTPUT);

  delay(2000);
  lcd.clear();

}


void loop(){

  int val = analogRead(A0);

  Serial.println(val);


  lcd.setCursor(0, 0);
  lcd.print("Alcohol Level:");

  lcd.setCursor(0, 1);
  lcd.print(val);

  if(val > 150) digitalWrite(buzzer,HIGH);

  else digitalWrite(buzzer,LOW);

  delay(1000);
  lcd.clear();





}
