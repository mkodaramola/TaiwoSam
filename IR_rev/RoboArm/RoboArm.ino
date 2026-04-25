#include <IRremote.h>
#include <Servo.h>

Servo ctrl;

const int mt = 10;
int IRpin = 4;
int vert = 0;
int hor = 0;
IRrecv ir(IRpin);

decode_results sig;

 void setup(){
  
  Serial.begin(9600);
  ctrl.attach(6);

  ctrl.write(0);
  
  pinMode(mt,OUTPUT);
  
  ir.enableIRIn();
  Serial.print("IR Enabling");
  for(int j=0;j<3;j++){
      Serial.print(".");
      delay(80);
    }
    Serial.println("\n\nIR Enabled.");
  
  }

  void loop(){
    


    if (ir.decode(&sig))
        {
          Serial.println(sig.value);


            if (sig.value == 2694879335){

                vert += 10;
                              ctrl.write(vert);

              
              }

             if (sig.value == 2694883415){
                
                vert -= 10;

                              ctrl.write(vert);

              
              } 

              if (sig.value == 2694863015){
                
                  analogWrite(mt, 200);
                
                }

                if (sig.value == 2694846695){

                  digitalWrite(mt,LOW);
                  
                  }




                    
            
          ir.resume();
        }
          }
        
  

    
