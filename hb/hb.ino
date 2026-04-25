#include <SoftwareSerial.h>

const int pulsePin = A0; // Pin where the pulse sensor is connected

SoftwareSerial BTSerial(2, 3); // RX, TX

int pulseLevel = 0;
int pulseDuration = 0;
int pulseStartTime = 0;
int pulseEndTime = 0;
int BPM = 0;

void setup() {
  pinMode(pulsePin, INPUT);

  Serial.begin(9600);
  BTSerial.begin(9600);

  BTSerial.println("Bluetooth initialized!");
}

void loop() {
  pulseLevel = digitalRead(pulsePin);

  if (pulseLevel == HIGH) {
    pulseStartTime = millis();
    while (pulseLevel == HIGH) {
      pulseLevel = digitalRead(pulsePin);
    }
    pulseEndTime = millis();
    pulseDuration = pulseEndTime - pulseStartTime;
    BPM = 60000 / pulseDuration;

    BTSerial.print("BPM: ");
    BTSerial.println(BPM);
    Serial.println(BPM);
  }
}
