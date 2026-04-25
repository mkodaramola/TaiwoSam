/*
  Arduino Nano 33 BLE Blink Example

  This board has a built-in LED already connected
  internally on the board.

  LED_BUILTIN is a special name that refers
  to the onboard LED.

  The program turns the LED on and off repeatedly.
*/

int led = 13;   // LED 13 on the board

void setup()
{
  /*
    setup() runs once when the board starts.
  */

  // configure the LED pin as an output
  pinMode(led, OUTPUT);
}

void loop()
{
  /*
    loop() runs forever.

    Everything inside this function repeats.
  */

  // turn the LED ON
  digitalWrite(led, HIGH);

  // wait 1 second
  delay(500);

  // turn the LED OFF
  digitalWrite(led, LOW);

  // wait 1 second
  delay(500);
}