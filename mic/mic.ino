void setup() {
  // put your setup code here, to run once:
  pinMode(4,OUTPUT);
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
int val = analogRead(A0);

if (val > 300)
  digitalWrite(4,HIGH);
else
  digitalWrite(4,LOW);
 
Serial.println(val);  

}
