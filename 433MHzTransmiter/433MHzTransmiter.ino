#include <RH_ASK.h>
#include <SPI.h>

RH_ASK rf_driver;

const int buttonPin = 5;

void setup() {
  pinMode(buttonPin, INPUT);
  Serial.begin(9600);
  if (!rf_driver.init())
    Serial.println("RF module initialization failed");
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH) {
    char message[] = "Button Pressed";
    rf_driver.send((uint8_t*)message, strlen(message));
    rf_driver.waitPacketSent();
    Serial.println("Button state transmitted");
  }

  delay(100);
}
