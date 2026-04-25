#include <Servo.h>

Servo ctrl;
int ldr = A0;
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  ctrl.attach(8);

    ctrl.write(0);
  delay(5);
}
                    
 int angle = 0;          
void loop() {
  // put your main code here, to run repeatedly:
int val = analogRead(ldr);
          
  if (val >= 1022){
    delay( 20);
   jump();
    }
    else{
      ctrl.write(0);
      } 
                                                                     
Serial.println(val);
delay(50);

}

void jump(){
  
  ctrl.write(30);
  delay(100);
  
  }
