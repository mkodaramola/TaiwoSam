const int led = 9;

int lum = 0;
float battery_level = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
  Serial.begin(9600);

}
int i = 0;
void loop() {
  // put your main code here, to run repeatedly:

  battery_level = analogRead(A0);
  lum = analogRead(A1);

  Serial.print("Battery Level:"); Serial.println(battery_level);
  Serial.print("lum:"); Serial.println(lum);  

  analogWrite(led,i);
 i+=5;
Serial.println(i);
  delay(500);
 

  if (i >= 255) i=0;



}
