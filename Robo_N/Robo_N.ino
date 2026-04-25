#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// ================= LCD Setup =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Eye open
byte eyeOpen[8] = {
  B00000,
  B01110,
  B10001,
  B10101,
  B10001,
  B01110,
  B00000,
  B00000
};

// Eye closed
byte eyeClosed[8] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000
};

// Mouth
byte mouth[8] = {
  B00000,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
  B00000,
  B00000
};



// ================= Motor Driver Pins =================
#define ENA 4
#define ENB 19
#define IN1 18
#define IN2 17
#define IN3 16
#define IN4 5

// ================= Ultrasonic Sensor Pins =================
#define TRIG 12
#define ECHO 14

// ================= Servo Setup =================
Servo myservo;
#define SERVO_PIN 27

// Blink timing
unsigned long lastBlink = 0;
int blinkInterval = 1000; // ms
bool rc = 0;
// ================= Bluetooth Setup =================
BluetoothSerial SerialBT;

// ================= Function Prototypes =================
void blinkEyes();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
long readDistance();
void handleBluetoothControl();
void dance();    

void Mprint(String tx, byte r, byte c) {
    lcd.setCursor(c, r);
    lcd.print(tx);
}

int speed = 180;
byte mode = 0;

void setup() {
  // LCD
  lcd.init();
  lcd.backlight();

  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.createChar(2, mouth);

  lcd.setCursor(5, 0); lcd.write(byte(0)); 
  lcd.setCursor(10, 0); lcd.write(byte(1));
  lcd.setCursor(7, 1); lcd.write(byte(2));

  // Serial monitor + Bluetooth
  Serial.begin(9600);
  SerialBT.begin("ESP32_Robot"); // Bluetooth device name

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ultrasonic pins
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Servo
  myservo.attach(SERVO_PIN);
  myservo.write(90); // center position

  Serial.println("Robot ready! Connect via Bluetooth...");
}

void loop() {

  int dis = readDistance();

  unsigned long currentMillis = millis();

  // Handle LCD blinking
  if (currentMillis - lastBlink >= blinkInterval) {
    lastBlink = currentMillis;
    blinkEyes();
  }

  handleBluetoothControl();

  if (dis < 15 && mode == 1) mode = 0;

  if (mode == 1) moveForward(speed);
  
  else if (mode == 2) moveBackward(speed);
  else if (mode == 3 && rc == 1) { turnLeft(250); rc = 0; }
  else if (mode == 3) turnLeft(speed);
  else if (mode == 4 && rc == 1) { turnRight(250); rc = 0; }
  else if (mode == 4) turnRight(speed);
  else stopMotors();
  

  Serial.println(String());



}


// Handle Bluetooth commands (external function)

String inputBuffer = "";
unsigned long lastCharTime = 0;     // timestamp of last received character
const unsigned long msgTimeout = 20; // ms gap to consider end of message

void handleBluetoothControl() {
  // Read characters as they come
  char c;
  while (SerialBT.available()) {
    c = SerialBT.read();
    inputBuffer += c;
    lastCharTime = millis();   // reset timer on each char
  }



  // If buffer has data and no new char arrived for msgTimeout ms → process
  if (inputBuffer.length() > 0 && (millis() - lastCharTime > msgTimeout)) {
    processData(inputBuffer);
    inputBuffer = ""; 

  }

}

void showMessage(String msg) {
  lcd.clear();
  Mprint(msg, 0, 0);
  delay(5000);
  lcd.clear();
  // restore robot face
  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.createChar(2, mouth);
  lcd.setCursor(5, 0); lcd.write(byte(0));
  lcd.setCursor(10, 0); lcd.write(byte(1));
  lcd.setCursor(7, 1); lcd.write(byte(2));
}

void processData(String s) {
  
  String inputBuffer = s; 
  s.toLowerCase();

  // Movement & speed control
  if (s.indexOf("stop") != -1 && s.indexOf("don't") == -1) mode = 0;
  else if (s.indexOf("forward") != -1 && (s.indexOf("bit") != -1 || s.indexOf("beat") != -1) ) {
      moveForward(200);
      delay(1000);
    }

  else if (s.indexOf("forward") != -1) mode = 1;

  else if (((s.indexOf("back") != -1 || s.indexOf("reverse") != -1)) && (s.indexOf("bit") != -1 || s.indexOf("beat") != -1)) {
      moveBackward(200);
      delay(1000);
    }
  else if ((s.indexOf("back") != -1 || s.indexOf("reverse") != -1)) mode = 2;

  else if (s.indexOf("dance") != -1 || s.indexOf("party") != -1) {
  dance(); 
  }
  else if ((s.indexOf("rc") != -1) && s.indexOf("left") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) {
    mode = 3; rc = 1; }
  else if ((s.indexOf("rc") != -1) && s.indexOf("right") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) { 
    mode = 4; rc = 1; }

  else if ((s.indexOf("keep") != -1 || s.indexOf("continue") != -1) && s.indexOf("left") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) {
    mode = 3;  }
  else if ((s.indexOf("keep") != -1 || s.indexOf("continue") != -1) && s.indexOf("right") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) { 
    mode = 4; } 

  else if (s.indexOf("left") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) {
    turnLeft(200);
    delay(2000);
    }
  else if (s.indexOf("right") != -1 && (s.indexOf("mov") != -1 || s.indexOf("turn") != -1)) {
    turnRight(200);
    delay(2000);
    }

  else if (s.indexOf("fast") != -1 && s.indexOf("very") != -1) speed += 50;
  else if (s.indexOf("slow") != -1 && s.indexOf("very") != -1) speed -= 50;
  else if (s.indexOf("increase") != -1 || s.indexOf("fast") != -1) speed += 20;
  else if (s.indexOf("decrease") != -1 || s.indexOf("reduce") != -1 || s.indexOf("slow") != -1) speed -= 20;

  // Cap speed
  speed = constrain(speed, 0, 255);

  SerialBT.print("Speed: "); SerialBT.println(speed);

  // ==== Conversational replies ====
  if (s.indexOf("hello") != -1 || s.indexOf("hi") != -1) {
    showMessage("Hello sir");
  }
   else if (s.indexOf("what") != -1 && s.indexOf("up") != -1) {
    showMessage("I'm good, you?");
  }
  else if (s.indexOf("launch") != -1) {
    showMessage("Wow!Thats Great");
  }
  else if (s.indexOf("how are you") != -1 || s.indexOf("how far") != -1) {
    showMessage("I'm fine, thank you");
  }
  else if (s.indexOf("how is your day") != -1 || (s.indexOf("day") != -1 && s.indexOf("you") != -1)) {
    showMessage("My day is great");
  }
  else if (s.indexOf("what's up") != -1) {
    showMessage("I'm good!");
  }
  else if (s.indexOf("i love you") != -1 || s.indexOf("i like you") != -1) {
    showMessage("I love you too sir");
  }
  else if (s.indexOf("thank") != -1) {
    showMessage("You're welcome");
  }
  else if (s.indexOf("sorry") != -1) {
    showMessage("No problem sir");
  }
  else if (s.indexOf("good morning") != -1) {
    showMessage("Good morning sir");
  }
  else if (s.indexOf("good afternoon") != -1) {
    showMessage("Good afternoon sir");
  }
  else if (s.indexOf("good evening") != -1) {
    showMessage("Good evening sir");
  }
  else if (s.indexOf("who are you") != -1 || s.indexOf("what are you") != -1) {
    showMessage("I'm Toby,an AI robot");
  }
  else if (s.indexOf("joke") != -1) {
    showMessage("I'm bad at jokes");
  }
    else if (s.indexOf("bye") != -1 || s.indexOf("goodbye") != -1) {
    showMessage("Goodbye! See you later");
  }
  else if (s.indexOf("see you") != -1) {
    showMessage("See you soon!");
  }
  else if (s.indexOf("what is your name") != -1) {
    showMessage("My name is Toby");
  }
  else if (s.indexOf("age") != -1) {
    showMessage("I don't have an age like humans");
  }
  else if (s.indexOf("weather") != -1) {
    showMessage("I can't check the weather now, but hope it's nice outside!");
  }
  else if (s.indexOf("help") != -1) {
    showMessage("Sure! How can I help you?");
  }
  else if (s.indexOf("thank you") != -1 || s.indexOf("thanks") != -1) {
    showMessage("Anytime!");
  }
  else if (s.indexOf("morning") != -1) {
    showMessage("Morning! Hope you slept well");
  }
  else if (s.indexOf("afternoon") != -1) {
    showMessage("Good afternoon! How's your day?");
  }
  else if (s.indexOf("evening") != -1) {
    showMessage("Good evening! How was your day?");
  }
  else if (s.indexOf("tired") != -1) {
    showMessage("You should take a break and relax");
  }
  else if (s.indexOf("hungry") != -1) {
    showMessage("Maybe grab something to eat?");
  }
  else if (s.indexOf("bored") != -1) {
    showMessage("Want me to suggest something fun?");
  }
  else if (s.indexOf("music") != -1) {
    showMessage("I like music too! What's your favorite?");
  }
  else if (s.indexOf("game") != -1) {
    showMessage("I love games! Which one do you play?");
  }
  else if (s.indexOf("funny") != -1) {
    showMessage("I try! Want to hear something funny?");
  }
  else if (s.indexOf("robot") != -1) {
    showMessage("Yes, I'm a friendly AI robot");
  }
  else if (s.indexOf("who made you") != -1 || s.indexOf("creator") != -1) {
    showMessage("I was made by my talented developer");
  }
  else if (s.indexOf("sleep") != -1) {
    showMessage("I don't sleep, but you should rest if tired");
  }
  else if (s.indexOf("fun") != -1) {
    showMessage("Life is better with fun!");
  }
    else if (s.indexOf("sad") != -1) {
    showMessage("I'm here if you want to talk about it");
  }
  else if (s.indexOf("angry") != -1) {
    showMessage("Take a deep breath, it helps");
  }
  else if (s.indexOf("excited") != -1) {
    showMessage("That's awesome! What's making you excited?");
  }
  else if (s.indexOf("love") != -1) {
    showMessage("Love makes the world better!");
  }
  else if (s.indexOf("friend") != -1) {
    showMessage("I can be your virtual friend!");
  }
  else if (s.indexOf("school") != -1 || s.indexOf("study") != -1) {
    showMessage("Studying is important! Keep it up");
  }
  else if (s.indexOf("work") != -1) {
    showMessage("Work hard, but remember to rest too");
  }
  else if (s.indexOf("funny story") != -1) {
    showMessage("I have many! But they might be a little corny");
  }
  else if (s.indexOf("joke") != -1 || s.indexOf("laugh") != -1) {
    showMessage("Why did the robot go on vacation? Because it needed to recharge! 😄");
  }
  else if (s.indexOf("advice") != -1) {
    showMessage("Always do your best, learn from mistakes, and stay curious");
  }



  
  inputBuffer.trim();

   if (inputBuffer.equals("F")) {
      mode = 1;
    }
    else if (inputBuffer.equals("B")) {
      mode = 2;
    }
    else if (inputBuffer.equals("L")) {
      mode = 3;
    }
    else if (inputBuffer.equals("R")) {
      mode = 4;
    }
    else if (inputBuffer.equals("S")) {
      mode = 0;
    }
    else if (inputBuffer.equals("U")) {
      speed += 20;   // increase speed
    }
    else if (inputBuffer.equals("D")) {
      speed -= 20;   // decrease speed
    }
    else if (inputBuffer.equals("C")) {
      myservo.write(90);  // center servo
    }
    else if (inputBuffer.equals("X")) {
      long dist = readDistance();
      SerialBT.print("Distance: ");
      SerialBT.println(dist);
    }



}

// ================= Functions =================

// Blink Eyes
void blinkEyes() {
  // Close eyes
  lcd.createChar(0, eyeClosed);
  lcd.createChar(1, eyeClosed);
  lcd.setCursor(5, 0); lcd.write(byte(0));
  lcd.setCursor(10, 0); lcd.write(byte(1));
  
  delay(200);

  // Open eyes
  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.setCursor(5, 0); lcd.write(byte(0));
  lcd.setCursor(10, 0); lcd.write(byte(1));

  blinkInterval = random(1000, 5000); 
  Serial.println(blinkInterval);
}

// Motor control
void moveForward(int sp) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, sp); analogWrite(ENB, sp);
}

void moveBackward(int sp) {


    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, sp); analogWrite(ENB, sp);
}

void turnLeft(int sp) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, sp); analogWrite(ENB, sp);
}

void turnRight(int sp) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, sp); analogWrite(ENB, sp);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

// Ultrasonic
long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}


void dance() {
  


  lcd.clear();
  Mprint("Dancing", 0, 0);

  for (int i = 0; i < 2; i++) { // repeat twice for extra groove
    moveForward(255);
    delay(2000);
    moveBackward(255);
    delay(2000);
    turnLeft(255);
    delay(2000);
    turnRight(255);
    delay(2000);
    stopMotors();
    delay(500);
  }

  stopMotors();

    lcd.clear();
  // restore robot face
  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.createChar(2, mouth);
  lcd.setCursor(5, 0); lcd.write(byte(0));
  lcd.setCursor(10, 0); lcd.write(byte(1));
  lcd.setCursor(7, 1); lcd.write(byte(2));
  
}