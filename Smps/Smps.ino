int led = 13;
int pot;
void setup() {
  // put your setup code here, to run once:

  pinMode(led,OUTPUT);
  Serial.begin(9600);  
  
}

void loop() {
  // put your main code here, to run repeatedly:
pot = analogRead(A0);

Serial.println(pot);
Serial.print("ST- ");
Serial.println(analogRead(A1));

if (pot >= analogRead(A1)){
  digitalWrite(led,HIGH);
  }
  else
  digitalWrite(led,LOW);

delay(100);
  
}
