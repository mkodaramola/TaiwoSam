#include <Servo.h>

Servo s;

int led = 11;
 int angle;
void setup() {
  // put your setup code here, to run once:
     s.attach(9);
     pinMode(led, OUTPUT);
     
}

void loop() {
  // put your main code here, to run repeatedly:

  for(int i = 0;i<255;i+=4){

      analogWrite(led,i);

     angle = map(i,0,255,0,180);

      s.write(angle);

      delay (60);  
    
    }
      
      

}
