#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x3F,16,2); 
char data;
String text = "";
boolean tst = false;
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();
}

void loop() {
  // put your main code here, to run repeatedly:

  if(Serial.available()> 0){

  data = Serial.read();
  
  }
    

  
switch(data){
  case '\b':
          text = "";
         lcd.clear();
    break;
 default:
     text += data;
  lcd.setCursor(0,0);
  Serial.println(text);
  if(tst == false){
    lcd.print(text.substring(0,text.length()-2));
    } 
    else{
      lcd.print(text.substring(2,text.length()));
      } 
  
      tst = true;
  

  }
  

}
