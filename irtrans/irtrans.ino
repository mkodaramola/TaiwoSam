#include <IRremote.h>


IRsend irsend;
   char c = 'a';
   int b = 12;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(b,INPUT);
  pinMode(10,HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

  digitalWrite(10,HIGH);


     if (digitalRead(b) == HIGH){

         irsend.sendNEC(0xFF30CF, 32);
            
            delay(2000);
            Serial.println("Sent");
      
      
      }
      else ;
    

}
