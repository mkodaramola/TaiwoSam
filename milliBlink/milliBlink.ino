#include <IRremote.h>

IRrecv ir(4);

decode_results sig;


int leds[3] = {11,12,13};


void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);

    ir.enableIRIn();

    Serial.println("IR Enabled...");
   
   
    for(int i=0;i<3;i++){

          pinMode(leds[i],OUTPUT);
      
      }

  
}

void loop() {
  // put your main code here, to run repeatedly:

   if(ir.decode(&sig)){

        Serial.println(sig.value);

          if (sig.value == 16724175){

             for(;;){
              
               for (int i=0;i<3;i++){

      digitalWrite(leds[i], HIGH);

          delay(300);      
      }

       for (int i=2;i>=0;i--){

      digitalWrite(leds[i], LOW);

          delay(300);      
      }

ir.resume();
      delay(30);
              
              }

     
              
            }

            if(sig.value == 16718055){
              
              for(;;){

                 for (int i=0;i<3;i++){

      digitalWrite(leds[i], HIGH);

          delay(500);      
      }

       for (int i=0;i<3;i++){

      digitalWrite(leds[i], LOW);

          delay(500);      
      }

ir.resume();
      delay(50);
              
                
                }
              }
            

  ir.resume();
    
    }
  


}
