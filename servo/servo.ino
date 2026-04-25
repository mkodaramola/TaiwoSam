#include <Servo.h>

Servo yaw;
Servo protrusion;
Servo vertical;
Servo gripper;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  yaw.attach(2);
  protrusion.attach(3);
  vertical.attach(4);
  gripper.attach(5);
  
 

  for (int i=0;i<180;i++){
    yaw.write(i);
    protrusion.write(i);
    vertical.write(i);
    delay(20);
    }

      for (int i=180;i>0;i--){
    yaw.write(i);
    protrusion.write(i);
    vertical.write(i);
    delay(20);
    }







}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i=0;i<180;i++){
    yaw.write(i);
    protrusion.write(i);
    vertical.write(i);
    delay(35);
    }
      delay(1000);
      for (int i=180;i>0;i--){
    yaw.write(i);
    protrusion.write(i);
    vertical.write(i);
    delay(35);
    }


}
