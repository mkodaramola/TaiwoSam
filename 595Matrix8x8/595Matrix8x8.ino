x#define DATA 10
#define SHIFT 11
#define STORE 12

void store();

  int dec_K[8][2] = {{1,64},{239,32},{223,16},{191,8},{127,4},{247,16},{251,8},{253,4}};
  int dec_B[7][2] = {{1,64},{253,124},{243,2},{239,124},{159,2},{127,124}};

  boolean k = true;
  unsigned long curr=0;
  
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  pinMode(DATA,OUTPUT);
  pinMode(SHIFT,OUTPUT);
  pinMode(STORE,OUTPUT);

  shiftOut(DATA, SHIFT, LSBFIRST, 0);
  shiftOut(DATA, SHIFT, LSBFIRST, 0);
  store();
}
void loop() {

if((millis() - curr)>= 1000){
    curr = millis();
    k = !k;
   }


 if(k == true) K();
 else {
    B();
  }

    
}


void store(){
    digitalWrite(STORE,HIGH);
    delayMicroseconds(10);
    digitalWrite(STORE,LOW);
    delayMicroseconds(10);
}

void K(){
      for(int i=0;i<8;i++){
         for(int j=0;j<2;j++){
          shiftOut(DATA, SHIFT, LSBFIRST, dec_K[i][j]);
          }
         store();
      }
  }

  void B(){
            for(int i=0;i<7;i++){
         for(int j=0;j<2;j++){
          shiftOut(DATA, SHIFT, LSBFIRST, dec_B[i][j]);
          }
         store();
      }
    }
