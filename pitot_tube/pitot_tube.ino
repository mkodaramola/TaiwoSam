#include <Wire.h>

#define PITOT_ADDRESS 0x28 // Replace this with the actual I2C address of your pitot tube sensor

void setup() {
  Serial.begin(9600);
  Wire.begin();
}

void loop() {
  // Request 2 bytes of data from the pitot tube sensor
  Wire.requestFrom(PITOT_ADDRESS, 2);

  // Wait until data is available
  while (Wire.available() < 2);

  // Read the two bytes and combine them into a 16-bit value
  uint16_t value = Wire.read() << 8 | Wire.read();

  // Calculate airspeed using the formula provided by your pitot tube sensor datasheet
  

  // Print the airspeed value
  Serial.print("Airspeed: ");
  Serial.print(value);
  Serial.println(" m/s");

  // Add a delay before reading again
  delay(400);
}

float calculateAirspeed(uint16_t rawValue) {
  // You'll need to implement the specific formula provided by your pitot tube sensor datasheet
  // This is just a placeholder
  // Replace this with the actual formula
  return rawValue * 0.1; // Example conversion factor
}
