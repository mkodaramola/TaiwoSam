#include <Arduino_LSM6DSOX.h>


String str = "";
float P = 0, I = 0, D = 0;
float x, y, z;

// Motor A connections
int in1 = 4;
int in2 = 5;
int enA = 6;


// Motor B connections
int in3 = 8;
int in4 = 9;
int enB = 10;






void processData(String);
void IMUs_update();
void BTcomm();
void Mstop();
void backward(int);
void forward(int);
void left(int);
void right(int);







void setup() {
  Serial.begin(115200);  
  Serial1.begin(9600);     
  Serial.println("RP2040 ready");

  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in g's");
  Serial.println("X\tY\tZ");





}

void loop() {

  BTcomm();
  IMUs_update();

  





}






void BTcomm(){

  if (Serial1.available()) {
    str = Serial1.readString();
    Serial.println(str);
    processData(str);
    String pid = "P:" + String(P) + " I:" + String(I) + " D:" + String(D);
    Serial1.println(pid);


  }

  if (str.indexOf("forward") >= 0) {
    forward(128);   // call your function
  }
  else if (str.indexOf("backward") >= 0) {
    backward(128);   // call your function
  }
  else if (str.indexOf("left") >= 0) {
    left(128);   // call your function
  }
  else if (str.indexOf("right") >= 0) {
    right(128);   // call your function
  }
  else if (str.indexOf("stop") >= 0) {
    Mstop();   // call your function
  }


}

void IMUs_update(){

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);
  }


}



void processData(String s){

if (s.startsWith("p") || s.startsWith("P")){

  s = s.substring(1);

  P = s.toFloat();

}
else if (s.startsWith("i") || s.startsWith("I")){

  s = s.substring(1);

  I = s.toFloat();

}

else if (s.startsWith("d") || s.startsWith("D")){

  s = s.substring(1);

  D = s.toFloat();

}


}


void Mstop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}


void forward(int sp) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void backward(int sp) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void left(int sp) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void right(int sp) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}