#include <EEPROM.h>

#define EEPROM_SIZE 64
#define FLOAT_ADDR 0

float storedValue;

void setup() {
  Serial.begin(115200);

  // Start EEPROM
  EEPROM.begin(EEPROM_SIZE);



  // Read float from EEPROM
  EEPROM.get(FLOAT_ADDR, storedValue);

  Serial.print("Float read from EEPROM: ");
  Serial.println(storedValue);
}

void loop() {
  // Nothing here
  Serial.print("Float read from EEPROM: ");
  Serial.println(storedValue);
}
