#include <SoftwareSerial.h>
#include <Servo.h>


SoftwareSerial BTSerial(10, 11); // RX, TX


// Bluetooth pins 
const int btRX = 0;
const int btTX = 1;
int threshold = 200;
// Motor Driver Pins
const int motor1Pin1 = 4;
const int motor1Pin2 = 5;
const int motor2Pin1 = 6;
const int motor2Pin2 = 7;

byte ctrl = 0;

// Robotic Arm Servos
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Sensor Pins
const int mq2Pin = A0;  // MQ-2 gas sensor
const int metalDetectorPin = 2;  // Metal detector
const int buzzerPin = 13;  // Buzzer



void setup() {
  // Start serial communication at 9600 baud rate with Bluetooth module
  BTSerial.begin(38400);
  // Start serial communication at 9600 baud rate with Serial monitor (optional)
  Serial.begin(38400);
  
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(metalDetectorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  servo1.attach(3);
  servo2.attach(5);
  servo3.attach(6);
  servo4.attach(1);
  
  Serial.println("Waiting for Bluetooth input...");
}

void loop() {
  // Check if Bluetooth data is available

    if(Serial.available()) BTSerial.write(Serial.read());
  
  if (BTSerial.available()) {

     Serial.println((char)BTSerial.read());
    
  }


    // Read Sensors
  int gasValue = analogRead(mq2Pin);
  int metalDetected = digitalRead(metalDetectorPin);

  if (gasValue > threshold || metalDetected == HIGH) {
    digitalWrite(buzzerPin, HIGH);  // Activate Buzzer
  } else {
    digitalWrite(buzzerPin, LOW);  // Deactivate Buzzer
  }
  
  delay(1000);
}
