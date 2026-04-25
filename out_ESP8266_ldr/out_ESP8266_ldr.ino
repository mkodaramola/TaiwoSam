/*
  Analog Reading using an LDR (Light Dependent Resistor)

  An LDR changes its resistance based on light intensity.
  More light -> different voltage -> different analog value.

  The microcontroller reads this voltage using the ADC
  and converts it into a number.
*/

int ldr = A0;   // LDR connected to analog pin A0
int value = 0;  // variable to store the reading

void setup() {

  // start communication with the computer
  Serial.begin(9600);

  Serial.println("LDR Analog Reading Example");
}

void loop() {

  // read the analog voltage from the LDR
  value = analogRead(ldr);

  // print the value to the serial monitor
  Serial.print("LDR Value: ");
  Serial.println(value);

  // wait before the next reading
  delay(500);
}