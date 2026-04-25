#include <IRremote.h>
#include <Servo.h>
const int irPin = 2;
IRrecv irrecv(irPin);
decode_results res;


Servo ctrl;
int sval = 0;

int t11 = 3;
int t12 = 8;

int t21 = 11;
int t22 = 12;



void setup() {
  // put your setup code here, to run once:
  pinMode(t11,OUTPUT);
    pinMode(t21,OUTPUT);
    pinMode(t12,OUTPUT);
    pinMode(t22,OUTPUT);
    Serial.begin(9600);

    ctrl.attach(6);

    ctrl.write(175);


    irrecv.enableIRIn();

    Serial.println("IR Enabled!");

    
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(irrecv.decode(&res)){


    if (sval <= 0){
      sval = 5;
      }
       if (sval >= 180){
      sval = 175;
      }

      Serial.print("Servo Angle:");       Serial.println(sval);


  Serial.println(res.value);
    if(res.value == 2694863015)
    {
         digitalWrite(t21, LOW);
    digitalWrite(t22, LOW);
    digitalWrite(t11, HIGH);
    digitalWrite(t12, HIGH);

 
      }

      
            if (res.value == 2694856895)
            {

       digitalWrite(t21, HIGH);
    digitalWrite(t22, HIGH);
    digitalWrite(t11,LOW);
    digitalWrite(t12, LOW);
   
        
        }


        if (res.value == 2694846695)
        {
           digitalWrite(t21, LOW);
    digitalWrite(t22, LOW);
    digitalWrite(t11, LOW);
    digitalWrite(t12, LOW);

        }
         if (res.value == 2694879335)
         {

         sval -= 10;

        }

         if (res.value == 2694883415)
         {

        sval += 10;

        }


        

    irrecv.resume();
    }

    ctrl.write(sval);
    if (sval >= 180){
      sval = 175;
      }
   if (sval <= 0){
      sval = 5;
      }
}
