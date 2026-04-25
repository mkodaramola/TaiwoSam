#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

const int HX711_dout = 4; // MCU > HX711 dout pin
const int HX711_sck = 5; // MCU > HX711 sck pin

const int HX711_dout2 = 8; // MCU > HX711 dout pin
const int HX711_sck2 = 9; // MCU > HX711 sck pin

HX711_ADC LoadCell(HX711_dout, HX711_sck);

HX711_ADC LoadCell2(HX711_dout2, HX711_sck2);

//0x27

LiquidCrystal_I2C lcd(0x27,16,2);




const int calVal_eepromAddress = 0;
const int calVal_eepromAddress2 = 6;


int di = 0;
void setup() {

   lcd.init();
   lcd.init();

   lcd.backlight();

  lcd.setCursor(4,0);
  lcd.print("R.A.I.N");

  lcd.setCursor(1,1);
  lcd.print("TECHNOLOGIES...");
  delay(2000);
  lcd.clear();
  
  Serial.begin(9600);
  delay(10);
  Serial.println("Starting...");

  
  loadCell_setup();

  loadCell2_setup();
  
  
  
}
float v = 0;
float v2 =0;
int dt = 100;
void loop() {
 // LOADCELL 1
  static boolean newDataReady = false;
  const int serialPrintInterval = 0;
  unsigned long t = 0;

  if (LoadCell.update()) {
    newDataReady = true;
  }

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
       v = LoadCell.getData();
       v = v*10;
     if (di > 20){
       Serial.println("");
      Serial.print("Load cell 1: ");
      Serial.println(abs(v));
      lcd.setCursor(1,0);
      lcd.print(abs(v)/100);
      lcd.setCursor(5,0);
      lcd.print("Kg");
      lcd.setCursor(8,0);
      lcd.print("|");
    
      }
      
      newDataReady = false;
      t = millis();
    }
  }

  // LOADCELL 2
    static boolean newDataReady2 = false;
  const int serialPrintInterval2 = 0;
  unsigned long t2 = 0;
  if (LoadCell2.update()) {
    newDataReady2 = true;
  }

  if (newDataReady2) {
    if (millis() > t2 + serialPrintInterval2) {
       v2 = LoadCell2.getData();

      if(di > 20){
        Serial.print("Load cell 2: ");
      Serial.println(abs(v2));
      char bv2[10];
      dtostrf(abs(v2),6,2,bv2);
    lcd.setCursor(10,0);
    lcd.print(abs(v2)/1000);
    lcd.setCursor(14,0);
    lcd.print("Kg");
        }

        else{
        Serial.println("Wait...");
        }
       
      

      newDataReady2 = false;
      t2 = millis();
    }
  }
 
  di +=1;
  if (v > v2 && abs(v - v2) > 1000) {

  
    lcd.setCursor(0,1);
    lcd.print("STATE: UB[LEFT]");
  
  
  }
  else if (v2 > v && abs(v - v2) > 1000) {

    lcd.setCursor(0,1);
    lcd.print("STATE: UB[RIGHT]");
  
  
  }
  else {

   
    lcd.setCursor(0,1);
    lcd.print("STATE: BALANCED");
  
  
  }
  if (di > 20) dt = 200;
  delay(dt);
  lcd.clear();
}


void loadCell_setup(){
      LoadCell.begin();
  LoadCell.setReverseOutput();
  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingTime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  } else {
    float CalTare[2];
    
    EEPROM.get(calVal_eepromAddress, CalTare);
    
    LoadCell.setCalFactor(CalTare[0]);
    LoadCell.setTareOffset(CalTare[1]);
    
    Serial.println("Startup is complete");
    Serial.print("Calibration factor loaded from EEPROM: ");
    Serial.println(CalTare[0]);
    Serial.print("Tare offset loaded from EEPROM: ");
    Serial.println(CalTare[1]);
  }

  while (!LoadCell.update()) {
    // Wait for the first data update
  }

  LoadCell.tareNoDelay();
  Serial.println("Tare complete");
  Serial.println("You can now place your load on the load cell.");


}


void loadCell2_setup(){
      LoadCell2.begin();
  LoadCell2.setReverseOutput();
  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  LoadCell2.start(stabilizingTime, _tare);

  if (LoadCell2.getTareTimeoutFlag() || LoadCell2.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  } else {
    float CalTare[2];
    
    EEPROM.get(calVal_eepromAddress, CalTare);
    
    LoadCell.setCalFactor(CalTare[0]);
    LoadCell.setTareOffset(CalTare[1]);
    
    Serial.println("Startup is complete");
    Serial.print("Calibration factor loaded from EEPROM: ");
    Serial.println(CalTare[0]);
    Serial.print("Tare offset loaded from EEPROM: ");
    Serial.println(CalTare[1]);
  }

  while (!LoadCell2.update()) {
    // Wait for the first data update
  }

  LoadCell2.tareNoDelay();
  Serial.println("Tare complete");
  Serial.println("You can now place your load on the load cell.");


}
