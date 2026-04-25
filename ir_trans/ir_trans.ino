#include <IRremote.h>

const int switchPin = 7;

int buttonState =0;


IRsend irsend;

void setup() {

  pinMode(switchPin,INPUT);

}

void loop() {

  buttonState = digitalRead(switchPin);

    if (buttonState == HIGH){
      irsend.sendNEC(0xFEAB57,32);
      
      }

  delay(200);
}
