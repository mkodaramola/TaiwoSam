#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// ============================================================
// Toby ESP32 AI Robot
// Cleaned-up version of the original uploaded code
// ============================================================

// ================= LCD Setup =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
#define SERVO_PIN 27

Servo myservo;
BluetoothSerial SerialBT;

// ================= Robot State =================
int speedValue = 225;

// 0 = stop
// 1 = forward
// 2 = backward
// 3 = left
// 4 = right
byte mode = 0;

// RC = one-time short turn
bool rcTurn = false;

// ================= Blink State =================
unsigned long lastBlink = 0;
unsigned long blinkInterval = 1000;
const unsigned long blinkDuration = 200;

bool eyesAreClosed = false;

// ================= Bluetooth Buffer =================
String inputBuffer = "";
unsigned long lastCharTime = 0;

const unsigned long msgTimeout = 20;

// ================= Temporary Movement State =================
// Used for commands such as "move forward a bit".
bool timedMovementActive = false;
unsigned long timedMovementEnd = 0;

// 0 = none
// 1 = forward
// 2 = backward
// 3 = left
// 4 = right
byte timedMovementMode = 0;

// ================= LCD Message State =================
bool messageActive = false;
unsigned long messageEndTime = 0;

// ============================================================
// Function Prototypes
// ============================================================

void Mprint(const String &tx, byte row, byte col);

void setupFace();
void showMessage(const String &msg);
void updateMessage();

void blinkEyes();
void updateBlink();

void handleBluetoothControl();
void processData(String s);

void moveForward(int sp);
void moveBackward(int sp);
void turnLeft(int sp);
void turnRight(int sp);
void stopMotors();

void startTimedMovement(byte movementMode, int sp, unsigned long duration);
void updateTimedMovement();

long readDistance();

void dance();

void handleSingleCharacterCommand(char command);
void handleNaturalLanguageCommand(String s);

void restoreFace();

// ============================================================
// Setup
// ============================================================

void setup() {

  // ---------------- LCD ----------------
  lcd.init();
  lcd.backlight();

  setupFace();

  // ---------------- Serial ----------------
  // Serial.begin(9600);

  // ---------------- Bluetooth ----------------
  SerialBT.begin("ESP32_Robot");

  // ---------------- Motor Pins ----------------
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ---------------- Ultrasonic ----------------
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  digitalWrite(TRIG, LOW);

  // ---------------- Servo ----------------
  myservo.attach(SERVO_PIN);
  myservo.write(90);

  // ---------------- Initial State ----------------
  stopMotors();

  randomSeed(micros());

  Serial.println("Robot ready! Connect via Bluetooth...");
}

// ============================================================
// Main Loop
// ============================================================

void loop() {

  // Bluetooth must be checked frequently.
  handleBluetoothControl();

  // Update non-blocking features.
  updateBlink();
  updateMessage();
  updateTimedMovement();

  // Normal continuous movement.
  if (!timedMovementActive && !messageActive) {

    if (mode == 1) {
      moveForward(speedValue);
    }
    else if (mode == 2) {
      moveBackward(speedValue);
    }
    else if (mode == 3) {

      if (rcTurn) {
        turnLeft(250);
        rcTurn = false;
        mode = 0;
      }
      else {
        turnLeft(speedValue);
      }
    }
    else if (mode == 4) {

      if (rcTurn) {
        turnRight(250);
        rcTurn = false;
        mode = 0;
      }
      else {
        turnRight(speedValue);
      }
    }
    else {
      stopMotors();
    }
  }
}

// ============================================================
// LCD Face
// ============================================================

void setupFace() {

  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.createChar(2, mouth);

  lcd.setCursor(5, 0);
  lcd.write(byte(0));

  lcd.setCursor(10, 0);
  lcd.write(byte(1));

  lcd.setCursor(7, 1);
  lcd.write(byte(2));

  eyesAreClosed = false;
}

void restoreFace() {

  lcd.clear();

  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeOpen);
  lcd.createChar(2, mouth);

  lcd.setCursor(5, 0);
  lcd.write(byte(0));

  lcd.setCursor(10, 0);
  lcd.write(byte(1));

  lcd.setCursor(7, 1);
  lcd.write(byte(2));

  eyesAreClosed = false;
}

void Mprint(const String &tx, byte row, byte col) {

  lcd.setCursor(col, row);
  lcd.print(tx);
}

// ============================================================
// LCD Message
// ============================================================

void showMessage(const String &msg) {

  lcd.clear();

  // Limit the message to the LCD width.
  String displayText = msg;

  if (displayText.length() > 16) {
    displayText = displayText.substring(0, 16);
  }

  Mprint(displayText, 0, 0);

  messageActive = true;
  messageEndTime = millis() + 5000;

  stopMotors();
}

void updateMessage() {

  if (!messageActive) {
    return;
  }

  if ((long)(millis() - messageEndTime) >= 0) {
    messageActive = false;
    restoreFace();
  }
}

// ============================================================
// Eye Blinking
// ============================================================

void blinkEyes() {

  lcd.createChar(0, eyeClosed);
  lcd.createChar(1, eyeClosed);

  lcd.setCursor(5, 0);
  lcd.write(byte(0));

  lcd.setCursor(10, 0);
  lcd.write(byte(1));

  eyesAreClosed = true;
}

void updateBlink() {

  unsigned long currentMillis = millis();

  if (!eyesAreClosed) {

    if (currentMillis - lastBlink >= blinkInterval) {

      lastBlink = currentMillis;

      blinkEyes();

      // Schedule opening of the eyes.
      lastBlink = currentMillis;
    }
  }
  else {

    if (currentMillis - lastBlink >= blinkDuration) {

      lcd.createChar(0, eyeOpen);
      lcd.createChar(1, eyeOpen);

      lcd.setCursor(5, 0);
      lcd.write(byte(0));

      lcd.setCursor(10, 0);
      lcd.write(byte(1));

      eyesAreClosed = false;

      blinkInterval = random(1000, 5000);
      lastBlink = currentMillis;
      lcd.clear();
    }
  }
}

// ============================================================
// Bluetooth Input
// ============================================================

void handleBluetoothControl() {

  while (SerialBT.available()) {

    char c = (char)SerialBT.read();

    // Ignore CR and LF because many Bluetooth terminal apps
    // automatically append them.
    if (c == '\r' || c == '\n') {
      continue;
    }

    inputBuffer += c;
    lastCharTime = millis();

    // Avoid uncontrolled String growth.
    if (inputBuffer.length() > 200) {
      inputBuffer.remove(0, inputBuffer.length() - 200);
    }
  }

  if (
    inputBuffer.length() > 0 &&
    millis() - lastCharTime > msgTimeout
  ) {

    String command = inputBuffer;
    inputBuffer = "";

    processData(command);
  }
}

// ============================================================
// Command Processing
// ============================================================

void processData(String s) {

  s.trim();

  if (s.length() == 0) {
    return;
  }

  // ----------------------------------------------------------
  // Single-character control commands
  // ----------------------------------------------------------

  if (s.length() == 1) {

    char command = toupper((unsigned char)s.charAt(0));

    if (
      command == 'F' ||
      command == 'B' ||
      command == 'L' ||
      command == 'R' ||
      command == 'S' ||
      command == 'U' ||
      command == 'D' ||
      command == 'C' ||
      command == 'X'
    ) {

      handleSingleCharacterCommand(command);
      return;
    }
  }

  // ----------------------------------------------------------
  // Natural language commands
  // ----------------------------------------------------------

  s.toLowerCase();

  handleNaturalLanguageCommand(s);

  speedValue = constrain(speedValue, 0, 255);

  SerialBT.print("Speed: ");
  SerialBT.println(speedValue);
}

// ============================================================
// Single Character Commands
// ============================================================

void handleSingleCharacterCommand(char command) {

  switch (command) {

    case 'F':
      mode = 1;
      rcTurn = false;
      timedMovementActive = false;
      break;

    case 'B':
      mode = 2;
      rcTurn = false;
      timedMovementActive = false;
      break;

    case 'L':
      mode = 3;
      rcTurn = false;
      timedMovementActive = false;
      break;

    case 'R':
      mode = 4;
      rcTurn = false;
      timedMovementActive = false;
      break;

    case 'S':
      mode = 0;
      rcTurn = false;
      timedMovementActive = false;
      stopMotors();
      break;

    case 'U':
      speedValue += 20;
      break;

    case 'D':
      speedValue -= 20;
      break;

    case 'C':
      myservo.write(90);
      break;

    case 'X': {

      long dist = readDistance();

      SerialBT.print("Distance: ");
      SerialBT.print(dist);
      SerialBT.println(" cm");

      break;
    }
  }

  speedValue = constrain(speedValue, 0, 255);

  SerialBT.print("Speed: ");
  SerialBT.println(speedValue);
}

// ============================================================
// Natural Language Commands
// ============================================================

void handleNaturalLanguageCommand(String s) {

  // ----------------------------------------------------------
  // STOP
  // ----------------------------------------------------------

  if (
    s.indexOf("stop") != -1 &&
    s.indexOf("don't") == -1 &&
    s.indexOf("dont") == -1
  ) {

    mode = 0;
    rcTurn = false;
    timedMovementActive = false;

    stopMotors();
  }

  // ----------------------------------------------------------
  // FORWARD A BIT
  // ----------------------------------------------------------

  else if (
    s.indexOf("forward") != -1 &&
    (
      s.indexOf("bit") != -1 ||
      s.indexOf("beat") != -1
    )
  ) {

    startTimedMovement(1, 200, 1000);
  }

  // ----------------------------------------------------------
  // FORWARD
  // ----------------------------------------------------------

  else if (s.indexOf("forward") != -1) {

    mode = 1;
    rcTurn = false;
  }

  // ----------------------------------------------------------
  // BACKWARD A BIT
  // ----------------------------------------------------------

  else if (
    (
      s.indexOf("back") != -1 ||
      s.indexOf("reverse") != -1
    ) &&
    (
      s.indexOf("bit") != -1 ||
      s.indexOf("beat") != -1
    )
  ) {

    startTimedMovement(2, 200, 1000);
  }

  // ----------------------------------------------------------
  // BACKWARD
  // ----------------------------------------------------------

  else if (
    s.indexOf("back") != -1 ||
    s.indexOf("reverse") != -1
  ) {

    mode = 2;
    rcTurn = false;
  }

  // ----------------------------------------------------------
  // DANCE
  // ----------------------------------------------------------

  else if (
    s.indexOf("dance") != -1 ||
    s.indexOf("party") != -1
  ) {

    dance();
  }

  // ----------------------------------------------------------
  // RC LEFT
  // ----------------------------------------------------------

  else if (
    s.indexOf("rc") != -1 &&
    s.indexOf("left") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    mode = 3;
    rcTurn = true;
  }

  // ----------------------------------------------------------
  // RC RIGHT
  // ----------------------------------------------------------

  else if (
    s.indexOf("rc") != -1 &&
    s.indexOf("right") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    mode = 4;
    rcTurn = true;
  }

  // ----------------------------------------------------------
  // KEEP / CONTINUE LEFT
  // ----------------------------------------------------------

  else if (
    (
      s.indexOf("keep") != -1 ||
      s.indexOf("continue") != -1
    ) &&
    s.indexOf("left") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    mode = 3;
    rcTurn = false;
  }

  // ----------------------------------------------------------
  // KEEP / CONTINUE RIGHT
  // ----------------------------------------------------------

  else if (
    (
      s.indexOf("keep") != -1 ||
      s.indexOf("continue") != -1
    ) &&
    s.indexOf("right") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    mode = 4;
    rcTurn = false;
  }

  // ----------------------------------------------------------
  // ONE-TIME LEFT
  // ----------------------------------------------------------

  else if (
    s.indexOf("left") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    startTimedMovement(3, 200, 2000);
  }

  // ----------------------------------------------------------
  // ONE-TIME RIGHT
  // ----------------------------------------------------------

  else if (
    s.indexOf("right") != -1 &&
    (
      s.indexOf("mov") != -1 ||
      s.indexOf("turn") != -1
    )
  ) {

    startTimedMovement(4, 200, 2000);
  }

  // ----------------------------------------------------------
  // SPEED
  // ----------------------------------------------------------

  else if (
    s.indexOf("fast") != -1 &&
    s.indexOf("very") != -1
  ) {

    speedValue += 50;
  }

  else if (
    s.indexOf("slow") != -1 &&
    s.indexOf("very") != -1
  ) {

    speedValue -= 50;
  }

  else if (
    s.indexOf("increase") != -1 ||
    s.indexOf("fast") != -1
  ) {

    speedValue += 20;
  }

  else if (
    s.indexOf("decrease") != -1 ||
    s.indexOf("reduce") != -1 ||
    s.indexOf("slow") != -1
  ) {

    speedValue -= 20;
  }

  speedValue = constrain(speedValue, 0, 255);

  // ----------------------------------------------------------
  // Conversational Replies
  // ----------------------------------------------------------

  handleConversation(s);
}

// ============================================================
// Conversation Handler
// ============================================================

void handleConversation(String s) {

  // More specific phrases should come before general words.

  if (
    s.indexOf("good morning") != -1
  ) {

    showMessage("Good morning sir");
  }

  else if (
    s.indexOf("good afternoon") != -1
  ) {

    showMessage("Good afternoon sir");
  }

  else if (
    s.indexOf("good evening") != -1
  ) {

    showMessage("Good evening sir");
  }

  else if (
    s.indexOf("how are you") != -1 ||
    s.indexOf("how far") != -1
  ) {

    showMessage("I'm fine, thank you");
  }

  else if (
    s.indexOf("how is your day") != -1
  ) {

    showMessage("My day is great");
  }

  else if (
    s.indexOf("what's up") != -1 ||
    (
      s.indexOf("what") != -1 &&
      s.indexOf("up") != -1
    )
  ) {

    showMessage("I'm good, you?");
  }

  else if (
    s.indexOf("hello") != -1 ||
    s == "hi" ||
    s.startsWith("hi ") ||
    s.endsWith(" hi")
  ) {

    showMessage("Hello sir");
  }

  else if (
    s.indexOf("i love you") != -1 ||
    s.indexOf("i like you") != -1
  ) {

    showMessage("I love you too sir");
  }

  else if (
    s.indexOf("thank you") != -1 ||
    s.indexOf("thanks") != -1 ||
    s.indexOf("thank") != -1
  ) {

    showMessage("You're welcome");
  }

  else if (
    s.indexOf("sorry") != -1
  ) {

    showMessage("No problem sir");
  }

  else if (
    s.indexOf("launch") != -1
  ) {

    showMessage("Wow! Thats Great");
  }

  else if (
    s.indexOf("who are you") != -1 ||
    s.indexOf("what are you") != -1
  ) {

    showMessage("I'm Toby, an AI robot");
  }

  else if (
    s.indexOf("what is your name") != -1 ||
    s.indexOf("what's your name") != -1
  ) {

    showMessage("My name is Toby");
  }

  else if (
    s.indexOf("age") != -1
  ) {

    showMessage("I don't have an age like humans");
  }

  else if (
    s.indexOf("weather") != -1
  ) {

    showMessage(
      "I can't check the weather now"
    );
  }

  else if (
    s.indexOf("help") != -1
  ) {

    showMessage("Sure! How can I help you?");
  }

  else if (
    s.indexOf("joke") != -1
  ) {

    showMessage("I'm bad at jokes");
  }

  else if (
    s.indexOf("funny story") != -1
  ) {

    showMessage(
      "I have many! They might be corny"
    );
  }

  else if (
    s.indexOf("laugh") != -1 ||
    s.indexOf("funny") != -1
  ) {

    showMessage(
      "Why did the robot go on vacation?"
    );
  }

  else if (
    s.indexOf("bye") != -1 ||
    s.indexOf("goodbye") != -1
  ) {

    showMessage("Goodbye! See you later");
  }

  else if (
    s.indexOf("see you") != -1
  ) {

    showMessage("See you soon!");
  }

  else if (
    s.indexOf("morning") != -1
  ) {

    showMessage("Morning! Hope you slept well");
  }

  else if (
    s.indexOf("afternoon") != -1
  ) {

    showMessage("Good afternoon! How's your day?");
  }

  else if (
    s.indexOf("evening") != -1
  ) {

    showMessage("Good evening! How was your day?");
  }

  else if (
    s.indexOf("tired") != -1
  ) {

    showMessage(
      "You should take a break and relax"
    );
  }

  else if (
    s.indexOf("hungry") != -1
  ) {

    showMessage(
      "Maybe grab something to eat?"
    );
  }

  else if (
    s.indexOf("bored") != -1
  ) {

    showMessage(
      "Want me to suggest something fun?"
    );
  }

  else if (
    s.indexOf("music") != -1
  ) {

    showMessage(
      "I like music too! What's your favorite?"
    );
  }

  else if (
    s.indexOf("game") != -1
  ) {

    showMessage(
      "I love games! Which one do you play?"
    );
  }

  else if (
    s.indexOf("robot") != -1
  ) {

    showMessage(
      "Yes, I'm a friendly AI robot"
    );
  }

  else if (
    s.indexOf("who made you") != -1 ||
    s.indexOf("creator") != -1
  ) {

    showMessage(
      "I was made by my talented developer"
    );
  }

  else if (
    s.indexOf("sleep") != -1
  ) {

    showMessage(
      "I don't sleep, but you should rest"
    );
  }

  else if (
    s.indexOf("fun") != -1
  ) {

    showMessage(
      "Life is better with fun!"
    );
  }

  else if (
    s.indexOf("sad") != -1
  ) {

    showMessage(
      "I'm here if you want to talk about it"
    );
  }

  else if (
    s.indexOf("angry") != -1
  ) {

    showMessage(
      "Take a deep breath, it helps"
    );
  }

  else if (
    s.indexOf("excited") != -1
  ) {

    showMessage(
      "That's awesome! What's exciting?"
    );
  }

  else if (
    s.indexOf("love") != -1
  ) {

    showMessage(
      "Love makes the world better!"
    );
  }

  else if (
    s.indexOf("friend") != -1
  ) {

    showMessage(
      "I can be your virtual friend!"
    );
  }

  else if (
    s.indexOf("school") != -1 ||
    s.indexOf("study") != -1
  ) {

    showMessage(
      "Studying is important! Keep it up"
    );
  }

  else if (
    s.indexOf("work") != -1
  ) {

    showMessage(
      "Work hard, but remember to rest too"
    );
  }

  else if (
    s.indexOf("advice") != -1
  ) {

    showMessage(
      "Do your best, learn, and stay curious"
    );
  }
}

// ============================================================
// Timed Movement
// ============================================================

void startTimedMovement(
  byte movementMode,
  int sp,
  unsigned long duration
) {

  timedMovementMode = movementMode;
  timedMovementEnd = millis() + duration;
  timedMovementActive = true;

  mode = 0;
  rcTurn = false;

  sp = constrain(sp, 0, 255);

  switch (timedMovementMode) {

    case 1:
      moveForward(sp);
      break;

    case 2:
      moveBackward(sp);
      break;

    case 3:
      turnLeft(sp);
      break;

    case 4:
      turnRight(sp);
      break;

    default:
      stopMotors();
      timedMovementActive = false;
      break;
  }
}

void updateTimedMovement() {

  if (!timedMovementActive) {
    return;
  }

  if ((long)(millis() - timedMovementEnd) >= 0) {

    timedMovementActive = false;
    timedMovementMode = 0;

    stopMotors();
    return;
  }

  switch (timedMovementMode) {

    case 1:
      moveForward(200);
      break;

    case 2:
      moveBackward(200);
      break;

    case 3:
      turnLeft(200);
      break;

    case 4:
      turnRight(200);
      break;

    default:
      stopMotors();
      timedMovementActive = false;
      break;
  }
}

// ============================================================
// Motor Control
// ============================================================

void moveForward(int sp) {

  sp = constrain(sp, 0, 255);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, sp);
  analogWrite(ENB, sp);
}

void moveBackward(int sp) {

  sp = constrain(sp, 0, 255);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, sp);
  analogWrite(ENB, sp);
}

void turnLeft(int sp) {

  sp = constrain(sp, 0, 255);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, sp);
  analogWrite(ENB, sp);
}

void turnRight(int sp) {

  sp = constrain(sp, 0, 255);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, sp);
  analogWrite(ENB, sp);
}

void stopMotors() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ============================================================
// Ultrasonic Sensor
// ============================================================

long readDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  // Timeout prevents the ESP32 from getting stuck if
  // no echo is received.
  long duration = pulseIn(
    ECHO,
    HIGH,
    30000
  );

  if (duration == 0) {
    return 999;
  }

  long distance = duration * 0.0343 / 2;

  return distance;
}

// ============================================================
// Dance
// ============================================================

void dance() {

  // The original dance sequence was blocking.
  // It is kept as a complete routine because the dance command
  // is expected to finish before normal control resumes.

  timedMovementActive = false;
  mode = 0;

  lcd.clear();
  Mprint("Dancing", 0, 0);

  for (int i = 0; i < 2; i++) {

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

  restoreFace();
}