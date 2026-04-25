#include <Servo.h>

// Create servo object
Servo myServo;

// Pin connected to servo signal wire
#define SERVO_PIN PC7   // you can use another PWM-capable pin

void setup() {
  // Attach servo to the pin
  myServo.attach(SERVO_PIN);

  myServo.write(160);   // Move servo to position
    delay(50);    
}

void loop() {

    // Sweep back from 180 to 0
  for (int pos = 160; pos >= 140; pos -= 5) {
    myServo.write(pos);
    delay(50);
  }

  // Sweep from 0 to 180 degrees
  for (int pos = 140; pos <= 160; pos += 5) {
    myServo.write(pos);   // Move servo to position
    delay(50);           // Wait for servo to reach
  }


}
