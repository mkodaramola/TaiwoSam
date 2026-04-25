/*
  ==========================================
  ESP8266 BLINK EXAMPLE
  ==========================================

  This program blinks an LED connected to pin D2 (GPIO4).

  What students learn here:
  1. What a digital output is
  2. How to control a pin HIGH or LOW
  3. How the Arduino program structure works
     - setup()
     - loop()
*/

int led = 4;    // GPIO4 is the same as D2 on many ESP8266 boards

void setup() {

  // setup() runs ONLY ONCE when the board powers up or resets

  // Set the LED pin as an OUTPUT
  // This tells the microcontroller we want to send voltage OUT
  pinMode(led, OUTPUT);

}

void loop() {

  // loop() runs forever after setup() finishes

  // Turn the LED ON
  // HIGH means the pin outputs 3.3V
  digitalWrite(led, HIGH);

  // Wait for 1 second (1000 milliseconds)
  delay(1000);

  // Turn the LED OFF
  // LOW means the pin outputs 0V
  digitalWrite(led, LOW);

  // Wait again for 1 second
  delay(1000);

}