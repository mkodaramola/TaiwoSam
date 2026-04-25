#include <Servo.h>

Servo s_h;  
Servo s_s;  
Servo s_r;

int toggle = 0;

byte sd = 10;
int GD =1000;

int x = 0;
int y = 0;


void setup() {

   Serial.begin(9600);
  
  s_h.attach(9); 
  s_r.attach(10);
  s_s.attach(11);


    s_h.write(120);
    s_r.write(123);
     s_s.write(120);

     delay(GD);

     pinMode(13,OUTPUT);
     
}
bool ON = false;
bool tst = false;
void loop() {

    toggle = analogRead(A0);
    Serial.println(toggle);


    if(toggle < 75 && tst == false){

          ON = !ON;

          Serial.println("Toggled");

          tst = true;
          

        delay(2000);
          
          
      
      }

  
    Serial.println(ON);
    
    digitalWrite(13,ON);

    if (ON) active();

    tst = false;
    
                          
  }


  void active(){
    
     moveIN();
    
     delay(GD);

     rotIN();

     delay(GD);

     zIN();

     delay(GD);

     rotOUT();

     delay(GD);


    
     moveOUT();

   

    delay(GD);

    tst = false;

    ON = false;
    
    }


 void moveIN(){

  for (int pos = 120; pos >= 45; pos -= 1) { 
    s_h.write(pos);              
    delay(sd);                       
  }
 
  }

void moveOUT(){
  int pos2 = 150;
  byte mod = 40;
  for (int pos = 45; pos <= 120; pos += 1) { 
    s_h.write(pos);  
    s_s.write(pos2);  
    if (pos2 > 120) pos2-=1;
    
               
    delay(mod);
    if (mod > 27)mod-=1;                       
  }
 
  }

 void rotIN(){


  for (int pos = 123; pos >= 70; pos -= 1) { 
    s_r.write(pos);              
    delay(sd);                       
  }

   
  
  }

    void rotOUT(){

     for (int pos = 70; pos <= 123; pos += 1) { 
    s_r.write(pos);              
    delay(sd);                       
  }

    }

   void zIN(){
    
      for (int pos = 120; pos <= 150; pos += 1) { 
            s_s.write(pos);              
            delay(sd);                       
      }

  
      }

       void zOUT(){
    
      for (int pos = 150; pos >= 120; pos -= 1) { 
            s_s.write(pos);              
            delay(sd);                       
      }

  
      }



void CVctrl(){
  
  if(Serial.available()) {

      String c = Serial.readString();

   if (c.indexOf("|") > 0){

         int p = c.indexOf('|');     

      x = c.substring(0,p).toInt();

      y = c.substring(p+1).toInt();
      
     
      x = map(x,0,255,45,120);
      y = map(y,0,255,70,123);

           s_h.write(x);
           s_r.write(y);

    }
       

      
     
    }
  
  }
      
