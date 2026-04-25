/*
  ESP32 LDR Analog Reading Example

  In this example we read light intensity using an LDR
  (Light Dependent Resistor).

  An LDR changes its resistance based on light.
  More light -> different voltage -> different reading.

  The ESP32 converts the voltage into a number using
  its Analog-to-Digital Converter (ADC).

  The value will be printed to the Serial Monitor.
*/

int ldr = 34;     // LDR connected to GPIO34 (analog input)
int value = 0;    // variable to store the sensor reading

void setup()
{
  // start communication with the computer
  Serial.begin(115200);

  Serial.println("ESP32 LDR Analog Reading");
}

void loop()
{
  // read the analog voltage from the LDR
  value = analogRead(ldr);

  /*
    On ESP32 the ADC range is usually:

    0   -> minimum voltage
    4095 -> maximum voltage

    So the reading goes from 0 to 4095
  */

  // print the sensor value
  Serial.print("ldr value: ");
  Serial.println(value);

  // small delay before the next reading
  delay(500);
}