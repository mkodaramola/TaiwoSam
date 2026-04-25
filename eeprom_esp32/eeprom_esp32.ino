#include <EEPROM.h>

#define EEPROM_SIZE 64   // Allocate 64 bytes
#define FLOAT_ADDR 0     // Address to store the float

void setup() {
  Serial.begin(115200);

  // Start EEPROM
  EEPROM.begin(EEPROM_SIZE);

  float valueToStore = 23.75;

  // Write float into EEPROM
  EEPROM.put(FLOAT_ADDR, valueToStore);

  // Commit changes (important!)
  EEPROM.commit();

  Serial.println("Float written to EEPROM successfully!");
}

void loop() {
  // Nothing here
}
