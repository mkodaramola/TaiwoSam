#include <Servo.h>

// Create servo objects for each servo motor
Servo FL;
Servo BL;
Servo BR;
Servo FR;

void LF(int ang) {
  FL.write(ang);
}
void LB(int ang) {
  BL.write(ang);
}
void RB(int ang) {
  BR.write(180-ang);
}
void RF(int ang){
  FR.write(180-ang);
}

void setup() {
  // Attach each servo object to a specific pin
  FL.attach(2);
  BL.attach(3);
  BR.attach(4);
  FR.attach(5);

    LF(90);
    LB(90);
    RF(90);
    RB(90); 
}

void loop() {

   
   Forward4(200); 
    
}


void Forward3(int sp){
    LB(135);
    delay(400);
    RF(60);
    delay(500);
    LF(60);
    RB(110); 
    
    delay(sp);
    
    LF(135);
    LB(135);
    RF(45);  
    RB(45);
 
//    delay(sp);
//    LF(45);
//    LB(45);
//    RF(135);  
//    RB(135);
//    delay(sp); 

   
  }

void Forward2(int sp){
    LF(45);
    LB(45);
    RF(45);
    RB(45); 
    delay(sp);
    LF(135);
    LB(135);
    RF(45);  
    RB(45); 
    delay(sp);
    LF(45);
    LB(45);
    RF(135);  
    RB(135);
    delay(sp); 

   
  }
void Forward(int sp){
    LF(90);
    RF(90);
    LB(90);
    RB(90); 
    delay(sp);
    LF(90);
    RF(90);
    LB(45);
    RB(90); 
    delay(sp);
    LF(45);
    RF(90);
    LB(90);
    RB(90); 
    delay(sp);
    LF(90);
    
    RF(90);
    LB(90);
    RB(45); 
    delay(sp);
    LF(45);
    LB(90);
    RB(90);
    RF(60);
    delay(sp);

  }

  
  
void Forward4(int sp) {
    // Step 1: Move left front and right back legs forward
    LF(70);  // Left front moves forward
    RB(110); // Right back moves backward
    delay(sp);

    // Step 2: Move right front and left back legs forward
    RF(80);  // Right front moves forward
    LB(100); // Left back moves backward
    delay(sp);

    // Step 3: Move left front and right back legs backward to prepare for next step
    LF(100);  // Left front moves backward
    RB(80);   // Right back moves forward
    delay(sp);

    // Step 4: Move right front and left back legs backward to prepare for next step
    RF(100);  // Right front moves backward
    LB(80);   // Left back moves forward
    delay(sp);
}


void Forward5(int sp) {
  // Step 1: Move the front legs forward
  LF(60);
  RF(120);
  delay(sp);

  // Step 2: Move the back legs forward
  LB(60);
  RB(120);
  delay(sp);

  // Step 3: Bring the front legs back to the starting position
  LF(90);
  RF(90);
  delay(sp);

  // Step 4: Bring the back legs back to the starting position
  LB(90);
  RB(90);
  delay(sp);
}





  
