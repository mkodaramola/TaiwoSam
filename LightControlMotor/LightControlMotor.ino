int m = 7;

int ldr = A0;


int val;

void setup() {
  // put your setup code here, to run once:


  pinMode(m,OUTPUT);

  pinMode(ldr,INPUT);


}

void loop() {
  // put your main code here, to run repeatedly:


val = analogRead(ldr);

val = map(ldr,0,1023,0,255);


  if (val < 100){

    digitalWrite(m,HIGH);
    
    }
    else{
      digitalWrite(m,LOW);
      }
  



}
