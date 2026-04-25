#include <Stepper.h>

const int stepsPerRevolution = 200;  // Change this to match your stepper motor
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

void setup() {
  myStepper.setSpeed(100);  // Set the motor speed in RPM
}

void loop() {
  myStepper.step(90);      // Rotate 90 degrees clockwise
  delay(1000);             // Delay for stability (adjust as needed)
           // Delay for stability (adjust as needed)
}
