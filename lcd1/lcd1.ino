#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,4,5);


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16,2);
  lcd.backlight();

  lcd.print("Hello world");

      
}

void loop() {
  // put your main code here, to run repeatedly:
delay(2000);
      lcd.clear();
      lcd.setCursor(0,1);
      lcd.print("we");
delay(800);
}
