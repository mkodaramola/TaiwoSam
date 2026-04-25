// Pin Definitions
const int batteryPin = A0;
const int ldrPin = A1;
const int ledPins[] = {4, 5, 6, 7};  // Battery level indicators
const int lampPin = 9;              // LED Lamp via MOSFET

// Constants
const float batteryMin = 3.3;  // 0%
const float batteryMax = 4.2;   // 100%
const int adcResolution = 1023;
const float voltageRef = 5.0;   // Assuming 5V Arduino

// LDR Threshold (tune this based on your setup)
const int lightThreshold = 500;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) pinMode(ledPins[i], OUTPUT);
  pinMode(lampPin, OUTPUT);
}

void loop() {
  float batteryVoltage = readBatteryVoltage();
  int batteryPercentage = calculateBatteryPercentage(batteryVoltage);
  int lightLevel = 1024-analogRead(ldrPin);

  displayBatteryLevel(batteryPercentage);
  controlLamp(lightLevel, batteryPercentage);

  // For debugging
  Serial.print("Battery Voltage: "); Serial.print(batteryVoltage); Serial.print(" V, ");
  Serial.print("Battery %: "); Serial.print(batteryPercentage); Serial.print("%, ");
  Serial.print("Light Level: "); Serial.println(lightLevel);

  delay(1000);
}

// ===== Function Definitions =====

// Read battery voltage
float readBatteryVoltage() {
  int raw = analogRead(batteryPin);
  float voltage = (raw / float(adcResolution)) * voltageRef;
  return voltage+1.92;
}

// Convert voltage to percentage
int calculateBatteryPercentage(float voltage) {
  voltage = constrain(voltage, batteryMin, batteryMax);
  return int(((voltage - batteryMin) / (batteryMax - batteryMin)) * 100);
}

// Display battery level using LEDs
void displayBatteryLevel(int percentage) {

  if (percentage > 0) digitalWrite(ledPins[0], HIGH);       // 0–25%
  else digitalWrite(ledPins[0], LOW);

  if (percentage > 25) digitalWrite(ledPins[1], HIGH);      // 25–50%
  else digitalWrite(ledPins[1], LOW);

  if (percentage > 50) digitalWrite(ledPins[2], HIGH);      // 50–75%
  else digitalWrite(ledPins[2], LOW);
  
  if (percentage > 75) digitalWrite(ledPins[3], HIGH);      // 75–100%
  else digitalWrite(ledPins[3], LOW);
}

// Control lamp brightness based on light and battery
void controlLamp(int lightLevel, int batteryPercentage) {
  if (lightLevel < lightThreshold) {
    // Dark condition
    int brightness = map(batteryPercentage, 0, 100, 50, 255); // Adaptive PWM
    analogWrite(lampPin, brightness);
  } else {
    analogWrite(lampPin, 0); // Bright - Turn off lamp
  }
}
