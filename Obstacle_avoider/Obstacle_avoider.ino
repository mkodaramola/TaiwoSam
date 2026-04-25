
int Lm=9,Rm = 8;
int Lled = 1;
int Rled = 2;

int acc=0;

int Sensor(){

    int ultra = 7;

    pinMode(ultra,OUTPUT);
    digitalWrite(ultra,LOW);
    delayMicroseconds(2);
    digitalWrite(ultra,HIGH);
    delayMicroseconds(5);
    digitalWrite(ultra,LOW);

    pinMode(ultra,INPUT);   
    int duration = pulseIn(ultra,HIGH);

    int cm= duration /48;

    return cm;
  
  
  
  }

  boolean Cprime(int x){

int p;
      if (x == 0){
        goto bs;
        }

      for(int i=2;i<x;i++){

      p = x%i;

      if (p == 0){

        return true;      
        }
        
        }
    bs:
    return false;
    
    
    }




  

  

void setup() {
  // put your setup code here, to run once:

  pinMode(Lm,OUTPUT);
  pinMode(Rm, OUTPUT);
    pinMode(Lled,OUTPUT);
  pinMode(Rled, OUTPUT);  

}

void loop() {
  // put your main code here, to run repeatedly:


  
unsigned int guage=0;




    if (Sensor() < 20){
      
        digitalWrite(Lm,LOW);
        digitalWrite(Rm,LOW);
        delay(100);
          if (Cprime(guage) == true){
            // Turn Left
             digitalWrite(Lm,LOW);
        digitalWrite(Rm,HIGH);
            }
            else{
              //Turn Right
               digitalWrite(Lm,true);
        digitalWrite(Rm,LOW);
              }
        delay(100);

        acc = 100;
      
      }

    
int cnt = constrain(acc,0,255);

analogWrite(Lm,cnt);
analogWrite(Rm,cnt);
    
  guage = guage + 1;
  

  acc=acc+10;


}
