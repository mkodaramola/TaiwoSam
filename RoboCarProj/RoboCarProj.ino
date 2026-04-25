#include <IRremote.h>


IRrecv ir(4);
decode_results res;
int mode = 1;
static int Mspeed=250;
byte motion = 0;
byte Omotion = 0;
byte LF = 5;
byte LB = 6;
byte RF = 9;
byte RB = 10;
byte irLed = 7;
int pd = A0;
int mic = A1;
byte RLed = 13;
byte OLed = 12;
byte rad;
boolean onMotion = false;

boolean enableMic = false;
void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600);
 ir.enableIRIn();
 pinMode(RLed,OUTPUT);
 pinMode(OLed, OUTPUT);
 pinMode(irLed, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:

if(mode == 1){

  digitalWrite(RLed,HIGH);
  digitalWrite(OLed,LOW);
  
 IrControllerM();

if (motion == 1) Forward();
else if (motion == 2) Backward();
else if (motion == 3) Left();
else if (motion == 4) Right();
else Stop();
  }
else if (mode == 2){
  
  digitalWrite(RLed,LOW);
  digitalWrite(OLed,HIGH);
  
  IrControllerPD();

  if(Omotion == 1) OForward();
  else if (Omotion == 2) Backward();
else if (Omotion == 3) Left();
else if (Omotion == 4) Right();
else Stop();
  }  

  }


void micTriggerO(){

int val = analogRead(mic);
Serial.println(val);

if(val < 445){

  switch(onMotion){
    case true:  
      Omotion = 4;
      delay(1000);
      break;
    case false: 
        Omotion = 1;
        delay(1000);
      break;

        default:
        ;
 
    }
  
  
  }

  }

  void micTriggerM(){

int val = analogRead(mic);
Serial.println(val);

if(val < 445){

  switch(onMotion){
    case true:  
      motion = 4;
      delay(1000);
      break;
    case false: 
        motion = 1;
        delay(1000);
      break;

        default:
        ;
 
    }
  
  
  }

  }




void doNothing(){}

void IrControllerM(){
if(enableMic == true) micTriggerM();
  if(ir.decode(&res)){

    long sig = res.value;
    
    Serial.println(sig);

      switch(sig){

        case 000:   
          enableMic = true;
          sig = 0;
          break;
        case 001: 
          enableMic = false;
          sig = 0;
          break;  
        case 3798652455: 
          mode = 1;
          Serial.println("Remote Control Mode");
          sig = 0;
          break;
        case 3545239723: 
          mode = 2;
          Serial.println("Obstacle Avoider Mode");
          sig = 0;
          break; 

        // Increase Speed  
        case 609727715: 
          Mspeed += 50;
          if(Mspeed > 255) Mspeed = 255;
          Serial.println(Mspeed);
          sig = 0;
          break;

          // Reduce Speed
         case -1813818205:  
         Mspeed -= 50;
         if (Mspeed < 0) Mspeed = 0;
         Serial.println(Mspeed);
         sig = 0;
         break; 

          // up
          case 1228432235: 
            motion = 1;
            onMotion = true;
            sig=0;
            break;

            
          // down
          case -1657252605:
            motion = 2;
            onMotion = true;
          sig=0;
            break;

            
          // Left
          case -2089382965:
            motion = 3;
            onMotion = true;
            sig=0;
            break;

            
          // Right
          case 701051847:
            motion = 4;
            onMotion = true;
            sig=0;
            break;

             // Disc View           
           case -432665849:
            motion = 0;
            onMotion = false;
            sig=0;
            break;
          default:
          ;  
        
        }

        
    ir.resume(); 
    
    }
  }


void IrControllerPD(){
if(enableMic == true) micTriggerO();
  if(ir.decode(&res)){

    long sig = res.value;

    Serial.println(sig);

      switch(sig){

        case 000:   
          enableMic = true;
          sig = 0;
          break;
        case 001: 
          enableMic = false;
          sig = 0;
          break; 
        
        case 3798652455: 
          mode = 1;
          Serial.println("Remote Control Mode");
          sig = 0;
          break;
        case 3545239723: 
          mode = 2;
          Serial.println("Obstacle Avoider Mode");
          sig = 0;
          break;  
        

        // Increase Speed  
        case 609727715: 
          Mspeed += 50;
          if(Mspeed > 255) Mspeed = 255;
          Serial.println(Mspeed);
          sig = 0;
          break;

          // Reduce Speed
         case -1813818205:  
         Mspeed -= 50;
         if (Mspeed < 0) Mspeed = 0;
         Serial.println(Mspeed);
         sig = 0;
         break; 

          // up
          case 1228432235: 
            Omotion = 1;
            onMotion = true;
             rad = random(2);
            sig=0;
            break;

            
          // down
          case -1657252605:
            Omotion = 2;
            onMotion = true;
          sig=0;
            break;

            
          // Left
          case -2089382965:
            Omotion = 3;
            onMotion = true;
            sig=0;
            break;

            
          // Right
          case 701051847:
            Omotion = 4;
            onMotion = true;
            sig=0;
            break;

             // Disc View           
           case -432665849:
            Omotion = 0;
            onMotion = false;
            sig=0;
            break;
          default:
          ;  
        
        }

        
    ir.resume(); 
    
    }
  }



  
void Forward(){
   digitalWrite(LF,HIGH);
            digitalWrite(RF,HIGH);
            digitalWrite(LB,0);
            digitalWrite(RB,0);
            Serial.println("Forward");
           
  }
void Backward(){
       digitalWrite(LF,0);            
          digitalWrite(RF,0);
            digitalWrite(LB,HIGH);
            digitalWrite(RB,HIGH);
           Serial.println("Backward");
          
  }
void Left(){
       digitalWrite(LF,HIGH);
            digitalWrite(RB,HIGH);
            digitalWrite(LB,0);
            digitalWrite(RF,0);
            Serial.println("Left");
            
  }

void Right(){
   digitalWrite(LB,HIGH);
   
            digitalWrite(RF,HIGH);
            digitalWrite(LF,0);
            digitalWrite(RB,0);
            Serial.println("Right");
            
            
  }
void Stop(){
    digitalWrite(LB,0);
            digitalWrite(RF,0);
            digitalWrite(LF,0);
            digitalWrite(RB,0);
            delay(2000);
            Serial.println("Off");

  }    
  
   
void OForward(){


  int pdVal = analogRead(A1);

  Serial.print("Photodiode: ");
  Serial.println(pdVal);

  if (pdVal < 1000) {
    if(rad == 0) Left();

    else Right();
 
    delay(5);
    }
else{
    analogWrite(LF,Mspeed);
            analogWrite(RF,Mspeed);
            analogWrite(LB,0);
            analogWrite(RB,0);
            Serial.println("Forward");
}

  }

  
