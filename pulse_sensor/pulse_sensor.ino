

const int pulsePin = A0; // Pulse sensor is connected to analog pin A0
const int ledPin = 13;   // LED is connected to digital pin 13

int pulseLevel; // Variable to store the pulse sensor value
int ledLevel;   // Variable to store the LED value

unsigned long time;  // Variable to store the time
unsigned long mdelay; // Variable to store the delay between beats

void setup() {
  // Initialize the serial port
  Serial.begin(9600);

  // Set the LED pin as an output
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Read the pulse sensor value
  pulseLevel = analogRead(pulsePin);

  // If the pulse sensor value is above a certain threshold...
  if (pulseLevel > 700) {
    // Turn on the LED
    ledLevel = HIGH;

    // Calculate the delay between beats
    mdelay = millis() - time;

    // Calculate the beats per minute
    int bpm = 60000 / mdelay;

    // Send the BPM value to the serial port
    Serial.println(bpm);

    // Update the time
    time = millis();
  } else {
    // Turn off the LED
    ledLevel = LOW;
  }

  // Update the LED
  digitalWrite(ledPin, ledLevel);
  
}
