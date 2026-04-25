
int led = 13;
boolean ledOn = HIGH;
void setup() {
  // put your setup code here, to run once:
  pinMode(led,OUTPUT);
  Serial.begin(9600);
  
    
}

void loop() {
  // put your main code here, to run repeatedly:
  int val = Serial.read();

  if(val == 'a'){
   ledOn = HIGH;
    }

  else if (val == 'b'){
   
  ledOn = LOW;

    }

  Serial.println(val);
    
  digitalWrite(led,ledOn);
 
  
}
