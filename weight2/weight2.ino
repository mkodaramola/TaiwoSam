
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
const int tareOffset_eepromAddress = 7;

const int calVal_eepromAddress2 = 0;
const int tareOffset_eepromAddress2 = 7;
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
       v = abs(v);
       v+= 2.5;
     if (di > 20){
       Serial.println("");
      Serial.print("Load cell 1: ");
      v = v/10;
      Serial.println(abs(v));
      if (v <= 4.5) v = 0;
      lcd.setCursor(1,0);
      lcd.print(v);
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
        v2 = abs(v2);
      if(di > 20){
        Serial.print("Load cell 2: ");
      Serial.println(v2);

    lcd.setCursor(10,0);
    v2 += 2.5;
    if (v2 <= 4.5) v2 = 0;
    lcd.print(v2);
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
  if (v > v2 && abs(v - v2) > 5) {

 
    lcd.setCursor(0,1);
    lcd.print("STATE: UB[RIGHT]");
  
  
  }
  else if (v2 > v && abs(v - v2) > 5) {

    lcd.setCursor(0,1);
    lcd.print("STATE: UB[LEFT]");

  
  }
  else if(v + v2 > 100) {

    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("please");
    lcd.setCursor(2,1);
    lcd.print("no more");
    
    } 
  else {

   
    lcd.setCursor(0,1);
    lcd.print("STATE: BALANCED");
  
  
  }
  if (di > 20) dt = 500;
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
    float calibrationFactor;
    float tareOffset;
    
    EEPROM.get(calVal_eepromAddress, calibrationFactor);
    EEPROM.get(tareOffset_eepromAddress, tareOffset);
    
    LoadCell.setCalFactor(calibrationFactor);
    LoadCell.setTareOffset(tareOffset);

//    Calibration value -2026.44 saved to EEPROM address: 0
//Tare offset value 7795805.00 saved to EEPROM address: 5

    
    Serial.println("Startup is complete");
    Serial.print("Calibration factor loaded from EEPROM: ");
    Serial.println(calibrationFactor);
    Serial.print("Tare offset loaded from EEPROM: ");
    Serial.println(tareOffset);
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
    float calibrationFactor;
    float tareOffset;
    
    EEPROM.get(calVal_eepromAddress2, calibrationFactor);
    EEPROM.get(tareOffset_eepromAddress2, tareOffset);
    
    LoadCell2.setCalFactor(calibrationFactor);
    LoadCell2.setTareOffset(tareOffset);


    
    Serial.println("Startup is complete");
    Serial.print("Calibration factor loaded from EEPROM: ");
    Serial.println(calibrationFactor);
    Serial.print("Tare offset loaded from EEPROM: ");
    Serial.println(tareOffset);
  }

  while (!LoadCell2.update()) {
    // Wait for the first data update
  }

  LoadCell2.tareNoDelay();
  Serial.println("Tare complete");
  Serial.println("You can now place your load on the load cell.");


}
