#include <IRremote.h>

const int RECV_PIN =4;

IRrecv irrecv(RECV_PIN);
int Lled = 13;
int Rled = 12;
  decode_results results;

void setup() {
  // put your setup code here, to run once:

Serial.begin(9600);
pinMode(Lled,OUTPUT);
pinMode(Rled,OUTPUT);

  
  irrecv.enableIRIn(); // Start the receiver

  Serial.print("IR Enabled.");

}

void loop() {
  // put your main code here, to run repeatedly:
        if(irrecv.decode(&results))
        {
          
          
          Serial.println(results.value);

          switch(results.value){

            case 1228432235:  
              digitalWrite(Lled,HIGH);
              digitalWrite(Rled,HIGH);
              break;
            case 2637714691:  
               digitalWrite(Lled,LOW);
              digitalWrite(Rled,LOW);
              break;
           case 701051847:  
               digitalWrite(Lled,LOW);
              digitalWrite(Rled,HIGH);
              break;
            case 2205584331:  
               digitalWrite(Lled,HIGH);
              digitalWrite(Rled,LOW);
              break;
             default:
             ;
               
            }


          



            
          irrecv.resume();
          
          }


}
