/*
  Arduino Mega Blink Example

  This program makes an LED blink repeatedly.

  An LED is connected to digital pin 13.
  The microcontroller will turn the LED ON and OFF
  by sending voltage to that pin.

  HIGH  = voltage is applied to the pin (LED turns on)
  LOW   = no voltage on the pin (LED turns off)

  The program structure in Arduino always has two parts:

  setup()  -> runs once when the board starts
  loop()   -> runs over and over forever
*/

int led = 13;   // the LED is connected to digital pin 13

void setup()
{
  /*
    setup() runs once when the board is powered on
    or when the reset button is pressed.
  */

  // configure the LED pin as an OUTPUT
  // this means the board will send signals OUT of this pin
  pinMode(led, OUTPUT);
}

void loop()
{
  /*
    loop() runs continuously after setup() finishes.
    The instructions inside loop() repeat forever.
  */

  // turn the LED ON
  digitalWrite(led, HIGH);

  // wait for 1 second (1000 milliseconds)
  delay(1000);

  // turn the LED OFF
  digitalWrite(led, LOW);

  // wait again for 1 second
  delay(1000);
}