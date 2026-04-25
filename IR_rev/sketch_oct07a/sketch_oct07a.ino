int mt = 9;
int potPin = 0;
int val;
void setup() {
  // put your setup code here, to run once:

  pinMode(mt, OUTPUT);
  pinMode(potPin, INPUT);
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  val = analogRead(potPin);

    analogWrite(mt, val);

  Serial.println(val);
  
}
