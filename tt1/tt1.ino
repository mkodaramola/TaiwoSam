int led = 10;
int push = 8;
boolean iniS = false;
boolean ledOn = false;
int p1;
void setup() {
pinMode(led, OUTPUT);
pinMode(push, INPUT);
Serial.begin(9600);
}

void loop() {
  
    if (digitalRead(push) == HIGH && iniS == 0){
       
    delay(10);

     p1 = digitalRead(push);
      }

    if (p1 == HIGH && iniS == 0){
       
        ledOn = !ledOn;
  Serial.print("3 ");
        iniS = 1;
      
      }

            if (digitalRead(push) == HIGH && iniS == 1){
  delay(5);
        ledOn = !ledOn;
    Serial.println("+ ");

        iniS = 0;
      
      }

      digitalWrite(led, ledOn);
 

}
