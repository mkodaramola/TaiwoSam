#include <IRremote.h>


const unsigned int right = 16769565;
const unsigned int left = 16753245;
const unsigned int mid = 16736925;

int m1 = 10;

int m2 = 11;

int irpin = 4;

IRrecv ir(irpin);

decode_results sig;

void setup() {
  // put your setup code here, to run once:
  pinMode(m1,OUTPUT);
  pinMode(m2,OUTPUT); 
  Serial.begin(9600);

 ir.enableIRIn();
  Serial.print("IR Enabling");
  for(int j=0;j<3;j++){
      Serial.print(".");
      delay(800);
    }
    Serial.println("\n\nIR Enabled.");

  
}

void loop() {
  // put your main code here, to run repeatedly:

      if(ir.decode(&sig)){
        
        
        Serial.println(sig.value);

     if (sig.value == 2694846695){

      digitalWrite(m1,HIGH);
           digitalWrite(m2,LOW);
      
      }

         if (sig.value == 2694863015){

          digitalWrite(m1,LOW);
            digitalWrite(m2,HIGH);
          
          }

         if (sig.value == 2694879335){

          digitalWrite(m1,HIGH);
            digitalWrite(m2, HIGH); 
          
          }

          if (sig.value == 2694883415){
            
               digitalWrite(m1,LOW);
            digitalWrite(m2, LOW); 
          
            }
          
        
        


        

        ir.resume();
        
        
        }


  
}
