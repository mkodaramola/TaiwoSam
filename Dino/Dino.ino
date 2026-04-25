#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F,16,2); 
byte Dino[8] = {
  
 
 0b11111,
 0b10000,
 0b10000,
 0b11111,
 0b10000,
 0b10000,  
 0b11111,
 0b00100
  };

void setup() {
  // put your setup code here, to run once:
lcd.init();    

   lcd.createChar(1,Dino);           
  lcd.begin(16,2);
  lcd.backlight();
  lcd.setCursor(5,0);
  lcd.write(1);
  lcd.setCursor(5,1);
  lcd.print("O");
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

}
