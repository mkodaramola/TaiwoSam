#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x24, 2, 16);


void setup() {
  // put your setup code here, to run once:

  lcd.init();               
  lcd.begin(16,2);
  lcd.backlight();
  lcd.setCursor(0,1);
  lcd.print("yello niger");
  delay(2000);
  lcd.clear();
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.clear();
  lcd.setCursor(0,0);
lcd.print("Hello world!");
}
