/*
  ESP32 Blink Example

  This program blinks an LED connected to a digital pin.

  The microcontroller sends voltage to the LED pin
  to turn it ON and removes the voltage to turn it OFF.

  Students learn:
  - what a digital output is
  - how the microcontroller controls hardware
*/

int led = 2;   // LED connected to GPIO2

void setup()
{
  // setup() runs once when the board starts

  // set the LED pin as an OUTPUT
  pinMode(led, OUTPUT);
}

void loop()
{
  // loop() runs repeatedly forever

  // turn LED on
  digitalWrite(led, HIGH);

  // wait for 1 second
  delay(1000);

  // turn LED off
  digitalWrite(led, LOW);

  // wait for 1 second
  delay(1000);
}