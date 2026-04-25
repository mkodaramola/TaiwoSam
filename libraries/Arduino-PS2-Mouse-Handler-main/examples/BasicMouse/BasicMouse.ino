#include <SoftwareSerial.h>

#define BT_TX 4  // HC-06 TX connected here
#define BT_RX 3  // HC-06 RX connected here
#define LED_PIN 13

SoftwareSerial BTSerial(BT_RX, BT_TX); // RX, TX (Arduino perspective)

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);      // For debugging with PC
  BTSerial.begin(9600);    // HC-06 default baud rate

  Serial.println("Waiting for Bluetooth command...");
}

void loop() {
  if (BTSerial.available()) {
    String command = BTSerial.readStringUntil('\n');
    command.trim(); // Remove any trailing newline or spaces

    Serial.print("Received via Bluetooth: ");
    Serial.println(command);

    if (command.equalsIgnoreCase("ON")) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED turned ON");
    } 
    else if (command.equalsIgnoreCase("OFF")) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED turned OFF");
    } 
    else {
      Serial.println("Unknown command");
    }
  }
}
