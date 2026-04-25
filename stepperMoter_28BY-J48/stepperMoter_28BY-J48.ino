#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9
// 1 = Clockwise
// 0 = Anti Clockwise
void setup() {
  // put your setup code here, to run once:

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

}
//GYRW

void loop() {
  // put your main code here, to run repeatedly:

Step(15*360,1,15);

delay(1000);

Step(15*360,0,15);

delay(1000);


}

void Step(float angle,boolean dir, int d){


  int s = (angle/360) * (48);
  
  for(int in=0;in<s;in++){
    
    
    if (dir){
      
      switch(in%4){
        case 0:
          digitalWrite(IN1,1);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;
         case 1:
          digitalWrite(IN1,0);
          digitalWrite(IN2,1);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;
          case 2:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,1);
          digitalWrite(IN4,0); 
          break;
          case 3:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,1);
          break;
          default: ;      
        }
    }
    else{
      switch(in%4){
        case 3:
            digitalWrite(IN1,1);
            digitalWrite(IN2,0);
            digitalWrite(IN3,0);
            digitalWrite(IN4,0);
          break;
         case 2:
          digitalWrite(IN1,0);
          digitalWrite(IN2,1);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;
          case 1:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,1);
          digitalWrite(IN4,0);
          break;
          case 0:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,1);
          break;
          default: ;
        }
      }
  
    
    delay(d);
    }
  
  
  }
