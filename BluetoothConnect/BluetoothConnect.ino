#include <SoftwareSerial.h>
SoftwareSerial BTSerial (2,3);

void setup() {
  // put your setup code here, to run once:
  pinMode(2,INPUT);
  pinMode(3,OUTPUT);
 Serial.begin (9600);
 Serial.println ("Enter AT Commands" );
 BTSerial.begin (9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  if (BTSerial.available()){
  
  Serial.write (BTSerial.read()) ;
   
 }
    //Serial.println(BTSerial.available());
  if (Serial.available ()){
      
     BTSerial.write(Serial.read ());
     
     }


}
