#include <SoftwareSerial.h>
SoftwareSerial BTSerial (2,3);


void setup () {
 // put your setup code here, to run once:
 Serial.begin (9600);
 BTSerial.begin (9600);
}
String cmd= "";
void loop () {    
  int val = analogRead(A0);

   //cmd= "";
   while (BTSerial.available()> 0){
     
      cmd+= (char)BTSerial.read();

    }
 
       Serial.println(cmd);
      
// 
//
//
//    BTSerial.println(val);
 


delay(100);
}
