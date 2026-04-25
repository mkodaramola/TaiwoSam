void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
pinMode(13,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int v = analogRead(A0);
Serial.println(v);
delay(200);
//if (v > 500) {digitalWrite(13,HIGH);delay(500);}
//else digitalWrite(13,0); 
}
