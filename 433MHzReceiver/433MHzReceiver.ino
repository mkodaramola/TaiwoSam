#include <RH_ASK.h>
#include <SPI.h>

RH_ASK rf_driver;

const int ledPin = 13;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  if (!rf_driver.init())
    Serial.println("RF module initialization failed");
}

void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (rf_driver.recv(buf, &buflen)) {
    // Message received successfully
    buf[buflen] = '\0'; // Null terminate the message

    if (strcmp((char*)buf, "Button Pressed") == 0) {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED turned ON");
    } else {
      digitalWrite(ledPin, LOW);
      Serial.println("LED turned OFF");
    }
  }
}
