#include <SoftwareSerial.h> 

SoftwareSerial BTSerial(2, 3); // RX | TX
int analogPin = A0;

void setup() {
  BTSerial.begin(9600);
  Serial.begin(9600);
}

void loop() {
  if (BTSerial.available()){
    BTSerial.write("AT"); // send command to check connection
    delay(100);
    if (BTSerial.available()){
      String response = BTSerial.readString();
      if (response == "OK") {
        int analogValue = analogRead(analogPin);
        BTSerial.println(analogValue);
        delay(100);
      }
      else {
        Serial.println("Error: Bluetooth module not connected");
      }
    }
  }
}
