
#include <Servo.h>

int d = 700;

Servo myservo;  // create Servo object to control a servo

void setup() {
  myservo.attach(9);  // attaches the servo on pin 9 to the Servo object

    myservo.write(45); 
                   
  delay(15); 
}

void loop() {
  

  myservo.write(15);

  delay(d);

  myservo.write(45);

  delay(d);


                        
}
