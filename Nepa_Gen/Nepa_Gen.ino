void Lblink(int light);
int nepa = 8;
int ctUse = 0;
long int gen = 12;
boolean ledOn = false;
long int prevT = 0;
long int gPrevT = 0;
void setup() {
  // put your setup code here, to run once:

   pinMode(nepa, INPUT);
  pinMode(gen,OUTPUT);

 digitalWrite(gen, LOW);

}

void loop() {
  // put your main code here, to run repeatedly:
  
long int currT = millis();
 

    if (digitalRead(nepa) != HIGH){
        int val;
        boolean tst = true;
    if (tst == true)  val = millis();
    
        while(millis() <=  val + 5000){
          Lblink(gen);
  
        }
    tst = false;
  
  digitalWrite(gen, HIGH);
      
    }
    else {
        digitalWrite(gen, LOW);

      }
  

}


void Lblink(int light){
 long int currT = millis();
 
 
   if(currT - prevT >= 1000){
    ledOn = !ledOn;
    prevT = currT;
    }

    digitalWrite(light,ledOn);

  }
