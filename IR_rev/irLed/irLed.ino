#include <IRremote.h>

const int mt = 10;
int IRpin = 4;
int mon = 0;
IRrecv ir(IRpin);

decode_results sig;

 void setup(){
  
  Serial.begin(9600);
 
  pinMode(mt,OUTPUT);

  ir.enableIRIn();
  Serial.print("IR Enabling");
  for(int j=0;j<3;j++){
      Serial.print(".");
      delay(800);
    }
    Serial.println("\n\nIR Enabled.");
  
  }

  void loop(){
    


    if (ir.decode(&sig))
        {
          Serial.println(sig.value);
          Serial.print("Analog Value: ");
              Serial.println(mon);
          

          if (sig.value == 2694879335 || sig.value == 4294967295){


               mon += 1;

            }

            
          if (sig.value == 2694883415){


               mon -= 1;

            }
            
            


            if (mon >= 255){
              mon = 0;
              }

           analogWrite(mt,mon);
            
          ir.resume();
          }
        
  
    }
