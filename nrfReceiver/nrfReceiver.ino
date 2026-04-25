#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(8, 7);  // CSN, CE

const int buttonPin = 12;
const int ledPin = 13;

bool buttonStateB = false;
bool buttonStateA = false;

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  radio.begin();
  radio.openWritingPipe(0xF0F0F0F0D2LL);  // Pipe address for Arduino A
  radio.openReadingPipe(1, 0xF0F0F0F0C1LL);  // Pipe address for Arduino A
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  buttonStateB = digitalRead(buttonPin);
  radio.write(&buttonStateB, sizeof(buttonStateB));

  if (radio.available()) {
    radio.read(&buttonStateA, sizeof(buttonStateA));
    digitalWrite(ledPin, buttonStateA);
  }
}
