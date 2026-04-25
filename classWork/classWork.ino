#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);
const int trigPin = 3;
const int echoPin = 2;


void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

  lcd.init(d);               
  lcd.begin(16,2);
  lcd.backlight();
}

void loop() {

  long duration, inches, cm;
 
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);

  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Dist(cm): ");

    
  lcd.setCursor(0,10);
  lcd.print(cm);

  delay(100);
}

long microsecondsToInches(long microseconds) {
return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
 
  return microseconds / 29 / 2;
}
