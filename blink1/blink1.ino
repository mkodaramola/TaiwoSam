 
int led = 8;



void setup() {
  // put your setup code here, to run once:

  pinMode(led,OUTPUT);


}

void loop(){
  
  digitalWrite(led,HIGH);
  delay(5);
  digitalWrite(led,0);
  delay(5);
  
  
  }
  
