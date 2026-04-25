int mode = 0;
byte LF = 5;
byte LB = 6;
byte RB = 9;
byte RF = 10;
int Mspeed = 0;

void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600);
 pinMode (LF, OUTPUT);
 pinMode (LB, OUTPUT);
 pinMode (RF, OUTPUT);
 pinMode (RB, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:

  
 char sig = Serial.read();

      switch(sig){

        case 'a': 
          Mspeed += 50;
          Serial.println(Mspeed);
          break;
         case 's':  
         Mspeed -= 50;
         Serial.println(Mspeed);
         
         break; 

          // up
          case '1': 
            analogWrite(LF,Mspeed);
            analogWrite(RF,Mspeed);
            analogWrite(LB,0);
            analogWrite(RB,0);
            Serial.println(1);
            break;
          // down
          case '2':
          analogWrite(LF,0);            
          analogWrite(RF,0);
            analogWrite(LB,Mspeed);
            analogWrite(RB,Mspeed);
                        Serial.println(2);

            break;
          // Left
          case '3':
            analogWrite(LF,Mspeed);
            analogWrite(RB,Mspeed);
            analogWrite(LB,0);
            analogWrite(RF,0);
            Serial.println(3);

            break;
          // Right
          case '4':
            analogWrite(LB,Mspeed);
            analogWrite(RF,Mspeed);
            analogWrite(LF,0);
            analogWrite(RB,0);
                        Serial.println(4);
               break;         
           case '0':
    analogWrite(LB,0);
            analogWrite(RF,0);
            analogWrite(LF,0);
            analogWrite(RB,0);

            break;
          default:
          ;  
        
        }

        

}


void doNothing(){}

void IrController(){


    
    
    
  
  
  
  }
