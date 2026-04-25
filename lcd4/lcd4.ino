#include <Wire.h> 
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
int i;
int p=0;
int n = 0;
void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.begin(16,2);
 
  Serial.begin(9600);
  // Print a message to the LCD.
  lcd.backlight();
  

 
}
void loop(){

  for(i=0;i<16;i++){
     lcd.setCursor(0,1);
    lcd.print(i*100/15); lcd.print("%");
    Serial.println(i);
    lcd.setCursor(i,0);
    lcd.print("-");
    delay(1000);
    

   
    
    }
  lcd.clear();
  }
