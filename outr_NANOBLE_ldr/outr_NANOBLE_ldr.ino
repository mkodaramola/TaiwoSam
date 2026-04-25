/*
  Nano 33 BLE LDR Analog Reading

  This example reads light intensity using an LDR.

  The LDR is connected in a voltage divider circuit
  so the amount of light changes the voltage at A0.

  The board reads this voltage and converts it
  into a digital number.
*/

int ldr = A0;     // LDR connected to analog pin A0
int value = 0;    // variable to store the sensor value

void setup()
{
  /*
    Start serial communication so the board
    can send data to the computer.
  */

  Serial.begin(9600);

  Serial.println("nano 33 ble ldr reading");
}

void loop()
{
  // read the voltage from the LDR
  value = analogRead(ldr);

  /*
    The Nano 33 BLE uses a 12-bit ADC.

    This means the range of values is:

    0 to 4095

    0    -> minimum voltage
    4095 -> maximum voltage
  */

  // send the reading to the serial monitor
  Serial.print("ldr value: ");
  Serial.println(value);

  // short pause before the next reading
  delay(500);
}