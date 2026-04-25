int audioIn = A0;
int audioOut = 9;
void setup() {
  // put your setup code here, to run once:

  pinMode(audioOut,OUTPUT);

  Serial.begin(9600);
  
}

void loop() {
  // put your main code here, to run repeatedly:

     int val = analogRead(audioIn);

      Serial.println(val);
     

}
