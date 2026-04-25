#include <Servo.h>


Servo ctrl;

int ang = 90;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  ctrl.attach(9);

}

void loop() {
  // put your main code here, to run repeatedly:

 
  char c = Serial.read();

  if (c == 'a') ang+=2;
  else if (c == 'l') ang-=2;
  
  
                                    
  Serial.println(ang);
  ctrl.write(ang);
  

}
