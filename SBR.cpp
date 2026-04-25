#include <Arduino_LSM6DSOX.h>

// -------- PID tunable constants --------
float P = 0.0, I = 0.0, D = 0.0;   

// -------- IMU variables --------
float x, y, z, gx, gy, gz;

// -------- PID working vars --------
unsigned long lastTime;
double Input, Output, Setpoint = 0;   // upright = 0°
double ITerm, lastInput;
int SampleTime = 10;                  // ms (100 Hz loop)
double outMin = -255, outMax = 255;

// -------- Complementary filter vars --------
double angle = 0;       // fused tilt angle
double accelAngle, gyroRate;
float alpha = 0.98;     // trust 98% gyro, 2% accel
unsigned long prevIMUTime;

// -------- Motor pins --------
int in1 = 4, in2 = 5, enA = 6;
int in3 = 8, in4 = 9, enB = 10;

// -------- Bluetooth --------
String str = "";

// =========================================
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);    // Bluetooth

  pinMode(enA, OUTPUT); pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  lastTime = millis();
  prevIMUTime = millis();

  Serial.println("Balancing Robot Ready");
}

// =========================================
void loop() {
  BTcomm();       // check for PID tuning commands
  IMUs_update();  // update filtered angle
  Compute();      // run PID
  driveMotors();  // drive based on PID output
}

// -------- Complementary Filter --------
void IMUs_update() {
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(x, y, z);
    IMU.readGyroscope(gx, gy, gz);

    unsigned long now = millis();
    float dt = (now - prevIMUTime) / 1000.0;
    prevIMUTime = now;

    // Tilt from accelerometer (pitch angle, deg)
    accelAngle = atan2(y, z) * 180 / PI;

    // Gyro rate around X (deg/s) — adjust axis if needed
    gyroRate = gx;

    // Complementary filter
    angle = alpha * (angle + gyroRate * dt) + (1 - alpha) * accelAngle;

    Input = angle;   // feed into PID
    Serial.println(Input);  // debug: stream filtered angle
  }
}

// -------- PID Compute (Improved) --------
void Compute() {
  unsigned long now = millis();
  int timeChange = (now - lastTime);
  if (timeChange >= SampleTime) {
    
    double error = Setpoint - Input;
    ITerm += (I * error);
    if (ITerm > outMax) ITerm = outMax;
    else if (ITerm < outMin) ITerm = outMin;

    double dInput = (Input - lastInput);

    Output = P * error + ITerm - D * dInput;
    if (Output > outMax) Output = outMax;
    else if (Output < outMin) Output = outMin;

    lastInput = Input;
    lastTime = now;
  }
}

// -------- Drive Motors --------
void driveMotors() {
  int pwm = abs((int)Output);
  if (pwm > 255) pwm = 255;

  if (Output > 0) forward(pwm);    // tilt forward → push forward
  else if (Output < 0) backward(pwm);
  else Mstop();
}

// -------- Bluetooth Commands --------
void BTcomm() {
  if (Serial1.available()) {
    str = Serial1.readString();
    processData(str);
    String pid = "P:" + String(P) + " I:" + String(I) + " D:" + String(D);
    Serial1.println(pid);   // send back current PID
  }
}

void processData(String s) {
  if (s.startsWith("p") || s.startsWith("P")) P = s.substring(1).toFloat();
  else if (s.startsWith("i") || s.startsWith("I")) I = s.substring(1).toFloat();
  else if (s.startsWith("d") || s.startsWith("D")) D = s.substring(1).toFloat();
}

// -------- Motor Helpers --------
void Mstop() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
  analogWrite(enA, 0); analogWrite(enB, 0);
}

void forward(int sp) {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  analogWrite(enA, sp); analogWrite(enB, sp);
}

void backward(int sp) {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  analogWrite(enA, sp); analogWrite(enB, sp);
}
