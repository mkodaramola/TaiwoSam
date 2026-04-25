
int led = 13;
int pir = 2;

void setup() {
  // put your setup code here, to run once:

  pinMode(led,OUTPUT);
  pinMode(pir,INPUT);
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:



    Serial.println(digitalRead(pir));


}
