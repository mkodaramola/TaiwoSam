int but = 7;
int Red_led = 3;
int led =13;
int White_led = 4;
int Blue_led = 5;
int fan = A0;
boolean temp = true;
boolean ledOn = false;
double fanSpeed = 0;
void setup() {
  // put your setup code here, to run once:

  pinMode(but, INPUT);
  pinMode(White_led, OUTPUT);
  pinMode(Red_led, OUTPUT);
  pinMode(Blue_led, OUTPUT);
  pinMode(led, OUTPUT);  
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  if(digitalRead(but) == HIGH) {
   delay(100);
    if (temp == true) {
        fanSpeed += 63.75;
        if(fanSpeed > 255) fanSpeed = 0;
        temp = false;
      }
  }
  else {
    temp = true;
    }

    if(fanSpeed == 255){
      digitalWrite(Red_led,HIGH);
      digitalWrite(White_led,LOW);
      digitalWrite(Blue_led,LOW);
    }
    else if(fanSpeed == 0){
      digitalWrite(Red_led,LOW);
      digitalWrite(White_led,LOW);
      digitalWrite(Blue_led,LOW);
    }

    else if(fanSpeed == 63.75){
      digitalWrite(Red_led,LOW);
      digitalWrite(White_led,LOW);
      digitalWrite(Blue_led,HIGH);
    }

    else if(fanSpeed == 127.5){
      digitalWrite(Red_led,LOW);
      digitalWrite(White_led,HIGH);
      digitalWrite(Blue_led,HIGH);
    }

    else if(fanSpeed == 191.25){
      digitalWrite(Red_led,HIGH);
      digitalWrite(White_led,HIGH);
      digitalWrite(Blue_led,LOW);
    }
    
    
    Serial.println(fanSpeed);
    analogWrite(fan,fanSpeed); 

    
 }
