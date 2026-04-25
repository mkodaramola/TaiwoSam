int led =13;
boolean button = LOW;

void setup() {
  // put your setup code here, to run once:

pinMode(led,OUTPUT);
pinMode(button,INPUT);

Serial.begin(9600);


}


void loop() {
  // put your main code here, to run repeatedly:
while(Serial.available() == 0);
  char k = Serial.read();

    if (k == '1'){
      
        button = !button;
        Serial.print("You Entered: "); Serial.println(k);
      }
     
        
                  digitalWrite(led,button);
        
        



}
