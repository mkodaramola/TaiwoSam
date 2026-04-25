#include <IRremote.h>

int Lir = 4;
int Rir = 6;
int Lled = 13;
int Rled = 9;
IRrecv irrecvL(Lir);
IRrecv irrecvR(Rir);
decode_results resL;

decode_results resR;
int ctL = 0;
int ctR = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(Lled,OUTPUT);
  pinMode(Rled,OUTPUT);
  Serial.begin(9600);
  irrecvL.enableIRIn();
  irrecvR.enableIRIn();

  Serial.println("Left IR Enable.");
    Serial.println("Right IR Enable.");
  

}

void loop() {
  // put your main code here, to run repeatedly:

  if(irrecvL.decode(&resL)){
      ctL++;
      --ctR;
      Serial.print("Left IR Active: "); Serial.print(resL.value); Serial.print("       L"); Serial.print(ctL); Serial.print("       R"); Serial.println(ctR);
     
    irrecvL.resume();
    }
     if(irrecvR.decode(&resR)){
      --ctL;
      ctR++;
      Serial.print("Right IR Active: "); Serial.print(resR.value); Serial.print("       L"); Serial.print(ctL); Serial.print("       R"); Serial.println(ctR);
      digitalWrite(Lled,LOW);
      digitalWrite(Rled,HIGH);
    irrecvR.resume();
    }


  if (ctL > ctR){
     digitalWrite(Lled,HIGH);
      digitalWrite(Rled,LOW);

      if (ctL- ctR >= 10){
        ctL = 0;
        ctR = 0;
        }
    }
     if (ctL < ctR){
     digitalWrite(Lled,LOW);
      digitalWrite(Rled,HIGH);

      if (ctR- ctL >= 10){
        ctL = 0;
        ctR = 0;
        }
    }

    

}
