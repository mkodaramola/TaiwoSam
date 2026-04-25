
int led = 10;

int but = 8;
int brg = 0;
boolean iniS = LOW;
boolean ledOn = false;

void setup(){
  Serial.begin(9600);

  pinMode(led,OUTPUT);
  pinMode(but,INPUT);
  
  
  }


    
   
    

  void loop(){

     
     
       if (digitalRead(but) == HIGH){

          analogWrite(led, brg);
        brg+=1;
        if (brg >= 255)
        brg = 0;
        }
        
delay(30);
  }
