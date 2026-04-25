
float ws=A0;

int val =0;

void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);


}

void loop() {
  // put your main code here, to run repeatedly:

val = analogRead(ws);

val = map(ws,0,1023,0,255);

  Serial.print("Sensor Value: "); Serial.println(val);
  
    


}
