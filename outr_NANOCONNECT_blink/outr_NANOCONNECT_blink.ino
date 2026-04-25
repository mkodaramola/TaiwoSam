/*
  Nano RP2040 Connect Blink Example

  The RP2040 board has pin 13.

  This program turns the LED ON and OFF
  continuously so we can see the board
  is running the program.
*/

int led = 13;   //  LED 13 on the board

void setup()
{
  /*
    setup() runs only once when
    the board starts or resets.
  */

  // configure the LED pin as an output
  pinMode(led, OUTPUT);
}

void loop()
{
  /*
    loop() runs repeatedly forever.
  */

  // turn the LED ON
  digitalWrite(led, HIGH);

  // wait for 1 second
  delay(1000);

  // turn the LED OFF
  digitalWrite(led, LOW);

  // wait for 1 second
  delay(1000);
}