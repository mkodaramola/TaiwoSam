#include <SoftwareSerial.h>

// Define the RX and TX pins for the SoftwareSerial
SoftwareSerial SerialBT(2, 3); // RX, TX

// Motor A connections
int enA = 9;
int in1 = 11;
int in2 = 10;

// Motor B connections
int enB = 4;
int in3 = 6;
int in4 = 5;

// Ultrasonic sensor connections
int trigPin = 7; // Trigger
int echoPin = 8; // Echo

int getDistance();
void Mstop();
void backward();
void forward();
void left();
void right();

unsigned long timer = 0;
uint32_t count = 0;

char cmd2 = '0';
boolean getC = false;

void setup() {
  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  SerialBT.begin(9600);

  Serial.println("Set up");  
}
void loop() {

       if (millis()-timer >= 200){
      count += 1;
      timer = millis();
      if (count >= 170) count = 170;
    }




  if (SerialBT.available()) {
  
   cmd2 = SerialBT.read();    
   

  }


  if (cmd2 == 'u') {
    
    forward(count);
    
    
  }
  else if (cmd2 == 'u') {
    backward(count);
  }
  else if (cmd2 == 'l') {
    left(170);
    delay(1400);
    cmd2 = '0';
  }
  else if (cmd2 == 'r'){
    right(170);
    delay(1400);
    cmd2 = '0';
  }
  else if (cmd2 == 's') Mstop();   

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

int getDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Set the trigger pin on for 10 microseconds
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pin, and calculate the distance
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (go and back)
  return distance;
}
