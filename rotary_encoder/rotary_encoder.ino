 #include <Servo.h>
 #define outputA 3
 #define outputB 4

 int counter = 0; 
 int aState;
 int aLastState;  

 Servo s;
 

 void setup() { 
   pinMode (outputA,INPUT);
   pinMode (outputB,INPUT);
   
   Serial.begin (115200);
   // Reads the initial state of the outputA
   aLastState = digitalRead(outputA);   
   s.attach(9);
 } 
unsigned long timer = 0;
int rot = 0;  
float sp = 0;

 void loop() { 
   aState = digitalRead(outputA); // Reads the "current" state of the outputA
   float volt = ((float)analogRead(A0)/1023.0)*5.0;
   // If the previous and the current state of the outputA are different, that means a Pulse has occured
   if (aState != aLastState){     
     // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
     if (digitalRead(outputB) != aState) { 
       counter ++;
      
     } else {
       counter --;
       if (counter <= 0) counter = 360;
     }

   } 
   aLastState = aState; // Updates the previous state of the outputA with the current state

   s.write(30);
   
   if (millis() - timer >= 1000){
        sp = counter/360.0;
        sp = sp*60;
    
     //Serial.println(counter);
        Serial.print(String(sp)); Serial.print(","); Serial.println(String(volt));
        counter = 0;
        
        timer = millis();
   }
   
 }
