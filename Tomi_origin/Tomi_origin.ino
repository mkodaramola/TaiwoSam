#include <Servo.h>
#include <SoftwareSerial.h>



Servo myservo;
SoftwareSerial sim800(2, 3);

int pos = 0;    
boolean fire = false;

/*-------defining Inputs------*/

const int Left_S = A1;
const int Right_S = A2;
const int Forward_S = A3;



#define LM1 4           // left motor
#define LM2 5           // left motor
#define RM1 6           // right motor
#define RM2 7           // right motor

#define pump 8
#define buzzer 13        // buzzer pin

#define enA 10
#define enB 11

#define MQ2 12          // MQ2 gas sensor

String number= "+2348142439130";

void Mstop();
void backward(int);
void forward(int);
void left(int);
void right(int);
void sendSMS(String,String);
void police_siren();

void setup()
{
  pinMode(Left_S, INPUT);
  pinMode(Right_S, INPUT);
  pinMode(Forward_S, INPUT);
  pinMode(MQ2, INPUT);   // MQ2 gas sensor input
  pinMode(LM1, OUTPUT);
  pinMode(LM2, OUTPUT);
  pinMode(RM1, OUTPUT);
  pinMode(RM2, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(buzzer, OUTPUT); // Buzzer output
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);

  sim800.begin(9600);
  Serial.begin(9600);
  
  myservo.attach(9);
  myservo.write(90);  
}

void put_off_fire()
{
    delay(500);

    digitalWrite(LM1, HIGH);
    digitalWrite(LM2, HIGH);
    digitalWrite(RM1, HIGH);
    digitalWrite(RM2, HIGH);
    
    digitalWrite(pump, HIGH); 
    delay(500);
    
    for (pos = 50; pos <= 130; pos += 1) {  
      myservo.write(pos);  
      delay(10);  
    }
    for (pos = 130; pos >= 50; pos -= 1) {  
      myservo.write(pos);  
      delay(10);
    }
  
    digitalWrite(pump, LOW);
    myservo.write(90);
    
    fire = false;

    sendSMS(number, "Fire out!");
}



void loop()
{
   myservo.write(90); //Sweep_Servo();  

   int mq2State = digitalRead(MQ2);  // Read MQ2 gas sensor state
   int fireDetected = digitalRead(Left_S) == 0 || digitalRead(Right_S) == 0 || digitalRead(Forward_S) == 0;

   if (fireDetected || mq2State == 0) {
     police_siren();  // Play police siren sound if fire or gas detected
     sendSMS(number,"Warning Alert! Fire Detected!!");
     
   } else {
     noTone(buzzer);   // Turn off the buzzer when no fire or gas detected
   }

    if (digitalRead(Left_S) == 1 && digitalRead(Right_S) == 1 && digitalRead(Forward_S) == 1) // If Fire not detected
    {
      // Do not move the robot
      Mstop();
      
    }
    else if (digitalRead(Forward_S) == 0) // If Fire is straight ahead
    {
      // Move the robot forward
      forward(100);
      fire = true;
    }
    else if (digitalRead(Left_S) == 0) // If Fire is to the left
    {
      // Move the robot left
      left(100);
    }
    else if (digitalRead(Right_S) == 0) // If Fire is to the right
    {
      // Move the robot right
      right(100);
    }

    delay(300); // Slow down the speed of robot

    while (fire == true)
    {
      put_off_fire();
    }
}









void sendSMS(String contactNumber, String message) {
  // Set SMS mode to text mode
  sim800.println("AT+CMGF=1"); 
  delay(100);
  
  // Send the AT command to send SMS to the contact number
  sim800.print("AT+CMGS=\"");
  sim800.print(contactNumber);
  sim800.println("\"");
  delay(100);

  // Send the message text
  sim800.println(message);
  delay(100);

  // End the SMS with Ctrl+Z (ASCII code 26)
  sim800.write(26);
  delay(1000);

  Serial.println("Message sent to " + contactNumber + ": " + message);
}


void police_siren()
{
  // Create a siren effect by alternating between two frequencies
  for (int i = 0; i < 10; i++) {  // Repeat 10 times
    tone(buzzer, 1000);  // Set frequency to 1000Hz
    delay(300);          // Hold for 300 milliseconds
    tone(buzzer, 500);   // Set frequency to 500Hz
    delay(300);          // Hold for 300 milliseconds
  }
  noTone(buzzer);  // Stop the tone
}


void Mstop() {
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}


void forward(int sp) {
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void backward(int sp) {
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, HIGH);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, HIGH);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void left(int sp) {
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, HIGH);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}

void right(int sp) {
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, HIGH);
  analogWrite(enA, sp);
  analogWrite(enB, sp);
}
