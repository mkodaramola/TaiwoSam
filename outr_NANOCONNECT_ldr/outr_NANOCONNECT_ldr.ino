/*
  Nano RP2040 Connect LDR Reading

  In this example we use an LDR to measure light.

  The LDR produces different voltages depending
  on how bright the environment is.

  The microcontroller reads this voltage and
  converts it into a digital value.
*/

int ldr = A0;     // LDR connected to analog pin A0
int value = 0;    // variable to store the reading

void setup()
{
  // start serial communication with the computer
  Serial.begin(9600);

  Serial.println("nano rp2040 connect ldr reading");
}

void loop()
{
  // read the analog voltage from the LDR
  value = analogRead(ldr);

  /*
    The RP2040 uses a 12-bit analog converter.

    This means the possible readings are:

    0 to 4095
  */

  // display the value in the Serial Monitor
  Serial.print("ldr value: ");
  Serial.println(value);

  // wait before the next reading
  delay(500);
}