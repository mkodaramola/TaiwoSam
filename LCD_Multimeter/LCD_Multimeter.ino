#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3C,16,4);


void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    lcd.init();
    lcd.begin(16,2);
    lcd.backlight();
    lcd.clear();
     lcd.setCursor(0,0);
    lcd.print("Welcome to");
    lcd.setCursor(0,1);
    lcd.print("Daratronics");
    for(int i=0;i<3;i++){
      delay(1200);
      lcd.setCursor(i+11,1);
      lcd.print(".");
      }
      delay(1200);

      lcd.clear();

       lcd.setCursor(2,0);
    lcd.print("Daratronics");
     lcd.setCursor(2,1);
    lcd.print("Multimeter");
      delay(2000);

      lcd.clear();
}
  String txt;
void loop() {
  // put your main code here, to run repeatedly:
  float volt = analogRead(A1);
    volt = (volt*5)/470.5;

    volt = constrain(volt,0,9);
    
   lcd.setCursor(0,0);
    lcd.print("Voltage: ");
     lcd.setCursor(9,0);
    lcd.print(volt);
    Serial.println(volt);
    delay(500);
    lcd.setCursor(9,0);
    lcd.print("");
    
}
