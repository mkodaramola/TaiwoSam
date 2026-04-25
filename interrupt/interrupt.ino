// Define the pin number
const int interruptPin = 15;
volatile int i = 0;
// Interrupt service routine (ISR)
void IRAM_ATTR handleInterrupt() {

  Serial.print("Interrupt triggered!"); Serial.println(i);
  i++;
  
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set pin 15 as input
  //pinMode(interruptPin, INPUT_PULLUP); // Use INPUT_PULLUP if you want to use the internal pull-up resistor
  pinMode(interruptPin,INPUT);
  // Attach the interrupt to the pin
  //attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
}

void loop() {
  // Main code here
  // The interrupt will trigger asynchronously when pin 15 goes from HIGH to LOW
  
  if(!digitalRead(interruptPin)){
      delay(500);
      Serial.print("Interrupt triggered!"); Serial.println(i);
    i++;
  }
}