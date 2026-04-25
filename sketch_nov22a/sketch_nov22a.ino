// Define the pin for the built-in LED
#define LED_BUILTIN 2

void setup() {
  // Initialize the built-in LED pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Turn the LED on
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000); // Wait for 1 second

  // Turn the LED off
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000); // Wait for 1 second
}
