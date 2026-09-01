#include <Arduino_LSM6DSOX.h>

// -------- PID tunable constants --------
float P = 30.0, I = 355, D = 1.5;   

// -------- IMU variables --------
float x, y, z, gx, gy, gz;

// -------- PID working vars --------
unsigned long lastTime;
double Input, Output, Setpoint = 1;   
double ITerm, lastInput;
int SampleTime = 10;                  
double outMin = -255, outMax = 255;

// -------- Complementary filter vars --------
double angle = 0;       
double accelAngle, gyroRate;
float alpha = 0.98;     
unsigned long prevIMUTime;


// -------- Motor pins --------
int in1 = 4, in2 = 5, enA = 6;
int in3 = 8, in4 = 9, enB = 10;

// -------- Bluetooth --------
String str = "";

// =========================================
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);  

   

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
  BTcomm();       
  IMUs_update();  
  Compute();      
  driveMotors();  
}

// -------- Complementary Filter --------
void IMUs_update() {
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(x, y, z);
    IMU.readGyroscope(gx, gy, gz);

    unsigned long now = millis();
    float dt = (now - prevIMUTime) / 1000.0;
    prevIMUTime = now;
    if (dt <= 0) return;

    
    accelAngle = atan2(y, z) * 180 / PI;

    gyroRate = gx;

    // Complementary filter
    angle = alpha * (angle + gyroRate * dt) + (1 - alpha) * accelAngle;

    Input = angle;   
    Serial.println(Input); 
  }
}


// -------- PID Compute (Improved) --------
void Compute() {
  unsigned long now = millis();
  float timeChange = (now - lastTime);
  if (timeChange >= SampleTime) {
    float dt = timeChange / 1000.0;
    if (dt <= 0) return;

    double error = Setpoint - Input;
    ITerm += (I * error * dt);
    if (ITerm > outMax) ITerm = outMax;
    else if (ITerm < outMin) ITerm = outMin;

    double dInput = (Input - lastInput)/dt;

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

  if ((Input > 80 && Input < 96) || (Input < -80 && Input > -96)){

    Mstop();
    
  }
    else {
   
    if (Output > 0) forward(pwm);   
    else if (Output < 0) backward(pwm);
    else Mstop();

   }

}

// -------- Bluetooth Commands --------
String inputBuffer = "";
unsigned long lastCharTime = 0;     // timestamp of last received character
const unsigned long msgTimeout = 20; // ms gap to consider end of message

void BTcomm() {
  // Read characters as they come
  while (Serial1.available()) {
    char c = Serial1.read();
    inputBuffer += c;
    lastCharTime = millis();   // reset timer on each char

    // Keep balancing while receiving chars
    IMUs_update();
    Compute();
    driveMotors();
  }

  // If buffer has data and no new char arrived for msgTimeout ms → process
  if (inputBuffer.length() > 0 && (millis() - lastCharTime > msgTimeout)) {
    processData(inputBuffer);
    inputBuffer = "";  // reset after processing

    // Send back PID values for confirmation
    String pid = "P:" + String(P) +
                 "\nI:" + String(I) +
                 "\nD:" + String(D) +
                 "\nSetpoint:" + String(Setpoint);
    Serial1.println(pid);
  }
}


void processData(String s) {
  if (s.startsWith("p") || s.startsWith("P")) P = s.substring(1).toFloat();
  else if (s.startsWith("i") || s.startsWith("I")) I = s.substring(1).toFloat();
  else if (s.startsWith("d") || s.startsWith("D")) D = s.substring(1).toFloat();
  else if (s.startsWith("t") || s.startsWith("T")) Setpoint = s.substring(1).toFloat();
  else if (s.startsWith("f") || s.startsWith("F")) Setpoint = 2.8;
  else if (s.startsWith("b") || s.startsWith("B")) Setpoint = -1.0;
  else if (s.startsWith("s") || s.startsWith("S")) Setpoint = 1;

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

void left(int sp) { 
digitalWrite(in1, LOW); digitalWrite(in2, HIGH); 
digitalWrite(in3, HIGH); digitalWrite(in4, LOW); 
analogWrite(enA, sp); analogWrite(enB, 0); 
} 

void right(int sp) { 
digitalWrite(in1, HIGH); digitalWrite(in2, LOW); 
digitalWrite(in3, LOW); digitalWrite(in4, HIGH); 
analogWrite(enA, 0); analogWrite(enB, sp); 
}
