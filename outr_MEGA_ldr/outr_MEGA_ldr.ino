/*
  Arduino Mega LDR Analog Reading

  In this example we measure light using an LDR
  (Light Dependent Resistor).

  An LDR changes resistance based on how much light
  falls on it.

  More light  -> different voltage
  Less light  -> different voltage

  The Arduino reads this voltage using the
  Analog to Digital Converter (ADC).

  The ADC converts voltage into a number.
*/

int ldr = A0;      // LDR connected to analog pin A0
int value = 0;     // variable to store the reading

void setup()
{
  /*
    Start communication between the Arduino
    and the computer.

    This allows us to send sensor values
    to the Serial Monitor.
  */

  Serial.begin(9600);

  Serial.println("arduino mega ldr reading example");
}

void loop()
{
  // read the analog value from the LDR
  value = analogRead(ldr);

  /*
    On Arduino Mega the analog range is:

    0   -> 0 volts
    1023 -> 5 volts

    So the sensor reading will be between
    0 and 1023.
  */

  // print the value to the serial monitor
  Serial.print("ldr value: ");
  Serial.println(value);

  // wait before taking another reading
  delay(500);
}