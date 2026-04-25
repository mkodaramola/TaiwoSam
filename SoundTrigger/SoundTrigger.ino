int mic = A0;
boolean ledOn = false;
int led = 13;
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  pinMode(led,OUTPUT);
  pinMode(mic,INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:

int val = analogRead(mic);

Serial.println(val);


}
