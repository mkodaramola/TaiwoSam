#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(8, 7);  // CSN, CE

const int buttonPin = 3;
const int ledPin = 2;

bool buttonStateA = false;
bool buttonStateB = false;

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  radio.begin();
  radio.openWritingPipe(0xF0F0F0F0C1LL);  // Pipe address for Arduino B
  radio.openReadingPipe(1, 0xF0F0F0F0D2LL);  // Pipe address for Arduino B
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  buttonStateA = digitalRead(buttonPin);
  radio.write(&buttonStateA, sizeof(buttonStateA));

  if (radio.available()) {
    radio.read(&buttonStateB, sizeof(buttonStateB));
    digitalWrite(ledPin, buttonStateB);
  }
}
