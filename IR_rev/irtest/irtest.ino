#include <IRremote.h>

int IRpin = 4;
int m1 = 9;

int m2 = 11;


IRrecv ir(IRpin);

decode_results sig;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  ir.enableIRIn();
  Serial.println("IR Enabled.");
}

void loop() {
  // put your main code here, to run repeatedly:

    if(ir.decode(&sig)){
      
      Serial.println(sig.value);

      if (sig.value == 2694879335){

          digitalWrite(m1,HIGH);
          digitalWrite(m2,HIGH);
        
        }

        if (sig.value == 2694883415) {
          digitalWrite(m1,LOW);
          digitalWrite(m2,LOW);
        }

           if (sig.value == 2694846695) {
          digitalWrite(m1,LOW);
          digitalWrite(m2,HIGH);
        }

           if (sig.value == 2694863015) {
          digitalWrite(m1,HIGH);
          digitalWrite(m2,LOW);
        }
        


        
        
        
      

      ir.resume();
      }

}
