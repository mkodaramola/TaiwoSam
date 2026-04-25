/*
  LDR Analog Reading Example

  An LDR (Light Dependent Resistor) changes resistance
  depending on how much light falls on it.

  The Arduino reads the voltage from the LDR
  and converts it into a number.
*/

int ldr = A0;
int value = 0;
int led = 13:
void setup()
{
  // start serial communication
  Serial.begin(9600);

  Serial.println("arduino uno ldr reading");

  pinMode(led, OUTPUT);

}

void loop()
{
  // read analog value from LDR
  value = analogRead(ldr);

  /*
    On Arduino Uno the ADC range is:

    0   -> 0 volts
    1023 -> 5 volts
  */

  Serial.print("ldr value: ");
  Serial.println(value);

  if (value < 500) {
    digitalWrite(led, HIGH);
  }
  else {
    digitalWrite(led, LOW);
  }

  delay(500);
}