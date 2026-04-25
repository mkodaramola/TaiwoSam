#define DISABLE -1


// DRV8313 control pins for 3 phases
const int IN1 = 2;
const int IN2 = 3;
const int IN3 = 4;

const int EN1 = 5;
const int EN2 = 6;
const int EN3 = 7;

// Motor control settings
int pwmDutyCycle = 128;       // 0 - 255 (controls torque)
int stepDelay = 1000;         // Delay between steps in µs (controls speed)
bool directionCW = true;      // true = CW, false = CCW

// PWM resolution setup
const int PWM_FREQ = 20000; // 20kHz PWM frequency

void setup() {
  // Set INx as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);

  // Set ENx as PWM outputs
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);

  // Start with all outputs low
  stopMotor();
}

void loop() {
  // Update motor direction and speed here
  trapezoidalCommutation(directionCW);  
}

// Trapezoidal commutation: 6-step sequence
void trapezoidalCommutation(bool cw) {
  if (cw) {
    step1(); delayMicroseconds(stepDelay);
    step2(); delayMicroseconds(stepDelay);
    step3(); delayMicroseconds(stepDelay);
    step4(); delayMicroseconds(stepDelay);
    step5(); delayMicroseconds(stepDelay);
    step6(); delayMicroseconds(stepDelay);
  } else {
    step6(); delayMicroseconds(stepDelay);
    step5(); delayMicroseconds(stepDelay);
    step4(); delayMicroseconds(stepDelay);
    step3(); delayMicroseconds(stepDelay);
    step2(); delayMicroseconds(stepDelay);
    step1(); delayMicroseconds(stepDelay);
  }
}

// --- 6-Step Commutation (Based on DRV8313 truth table) ---
void step1() { setPhase(HIGH, DISABLE , LOW); } // U+ V- WZ
void step2() { setPhase(HIGH, LOW, DISABLE); } // U+ W- VZ
void step3() { setPhase(DISABLE, LOW, HIGH); } // V+ W- UZ
void step4() { setPhase(LOW, DISABLE, HIGH); } // V+ U- WZ
void step5() { setPhase(LOW, HIGH, DISABLE); } // W+ U- VZ
void step6() { setPhase(DISABLE, HIGH, LOW); } // W+ V- UZ

// Set the phase logic and PWM
void setPhase(int a, int b, int c) {
  if (a == DISABLE) Disable(EN1);
  else if (b == DISABLE) Disable(EN2);
  else if (c == DISABLE) Disable(EN3);

  digitalWrite(IN1, (a < 0) ? 0 : a);
  digitalWrite(IN2, (b < 0) ? 0 : b);
  digitalWrite(IN3, (c < 0) ? 0 : c);
}


void Disable(int en){

    switch (en){

      case EN1:
        analogWrite(EN1, 0);
        analogWrite(EN2, pwmDutyCycle);
        analogWrite(EN3, pwmDutyCycle);
      break;

      case EN2:
        analogWrite(EN1, pwmDutyCycle);
        analogWrite(EN2, 0);
        analogWrite(EN3, pwmDutyCycle);
      break;

      case EN3:
        analogWrite(EN1, pwmDutyCycle);
        analogWrite(EN2, pwmDutyCycle);
        analogWrite(EN3, 0);
      break;

    } 

}



void stopMotor() {
  analogWrite(EN1, 0); digitalWrite(IN1, LOW);
  analogWrite(EN2, 0); digitalWrite(IN2, LOW);
  analogWrite(EN3, 0); digitalWrite(IN3, LOW);
}
