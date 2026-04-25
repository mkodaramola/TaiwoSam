int button = 7;

void setup() {
  // put your setup code here, to run once:
  pinMode(button, INPUT);
  Serial.begin(9600);
 

}

void loop() {
  // put your main code here, to run repeatedly:
  int b = digitalRead(button);
  if (b == HIGH) {
   Serial.println("RIGHT");
  }
 // Serial.println("LEFT");
  
  
}
