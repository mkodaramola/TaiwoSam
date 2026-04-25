const int pinA = 22;  // Output A
const int pinB = 23;  // Output B (inverted)
const unsigned long halfPeriod = 10000; // microseconds (10ms for 50Hz)

unsigned long lastToggleTime = 0;
bool state = false;

void setup() {
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  digitalWrite(pinA, LOW);
  digitalWrite(pinB, HIGH);  // Complementary to A
  lastToggleTime = micros();
}

void loop() {
  unsigned long currentTime = micros();

  if (currentTime - lastToggleTime >= halfPeriod) {
    state = !state;
    digitalWrite(pinA, state);
    digitalWrite(pinB, !state);
    lastToggleTime = currentTime;
  }
}
