int m1 = 9;

int m2 = 11;

void setup() {
  // put your setup code here, to run once:
  pinMode(m1,OUTPUT);
    pinMode(m2,OUTPUT);

}
int curr = millis();
int prev = 0;

int Tdif = curr - prev;
void loop() {
  // put your main code here, to run repeatedly:

digitalWrite(m1,HIGH);
delay(1000);
digitalWrite(m1,LOW);
delay(1000);


    
  
  
  
}

void both(){

  digitalWrite(m1,HIGH);

  digitalWrite(m2,HIGH);
    
  }

  void Fm1(){
      digitalWrite(m1,HIGH);
       digitalWrite(m2,LOW);

    }
  
  void Fm2(){
      digitalWrite(m1,LOW);
       digitalWrite(m2,HIGH);

    }

    
