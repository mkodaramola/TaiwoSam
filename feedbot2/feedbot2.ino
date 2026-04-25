#include <Servo.h>

Servo s_h;  
Servo s_s;  
Servo s_r;

int toggle = 0;

bool ledOn = false;
int led = 13;
byte sd = 10;
int GD =1000;
int x=0;
int y = 0;

int btn = 8;
bool swt = false;


void setup() {

  Serial.begin(9600);  
  s_h.attach(9); 
  s_r.attach(10);
  s_s.attach(11);
  


    s_h.write(120);
    s_r.write(123);
     s_s.write(120);
     

     delay(GD);

     pinMode(btn,INPUT);
     pinMode(led,OUTPUT);
     
}
bool ON = false;
bool tst = false;
void loop() {


  if (!digitalRead(btn)){

      swt = !swt;
      delay(1000);

      if (!swt){

    s_h.write(120);
    s_r.write(123);
     s_s.write(120);
        
        }
    
    }



  if (swt) workMode();

  else {

    toggle = analogRead(A0);
    Serial.println(toggle);


    if(toggle > 980){
      
          ON = true;    
          delay(1000);
                   
      }
    
    digitalWrite(led,ON);

    if (ON) active();

  
  }  

    
                          
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

    ON = false;
    
    }


 void moveIN(){

  for (int pos = 120; pos >= 35; pos -= 1) { 
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
    if (pos <=75)delay(sd);                       
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
            delay(2);                       
      }

  
      }

       void zOUT(){
    
      for (int pos = 150; pos >= 120; pos -= 1) { 
            s_s.write(pos);              
            delay(sd);                       
      }

  
      }


    int h_angle = 120;

    int s_angle = 120;
    

    void workMode(){
     
          s_r.write(20);
      
         if(Serial.available()) {
      
            String c = Serial.readString();

            if (c.equals("led_on")) digitalWrite(led, HIGH);
      
          if (c.equals("led_off")) digitalWrite(led, LOW);
      
            if (c.equals("down")){
      
                s_angle += 20;
      
                if (s_angle >= 180) s_angle = 180;
      
                s_s.write(s_angle);  

                
                
              }
            if (c.equals("up")){
      
                s_angle -= 20;
      
                if (s_angle <= 0) s_angle = 0;
      
                s_s.write(s_angle);

              
                   
                }
            if (c.equals("left")){
      
              h_angle += 20;
      
              if (h_angle >= 180) h_angle = 180;
              
                s_h.write(h_angle);
                   
                }
            if (c.equals("right")){
      
               h_angle -= 20;
              if (h_angle <= 0) h_angle = 0;
                s_h.write(h_angle);
                   
                }

                if (c.equals("fdown")){
      
                s_angle += 40;
      
                if (s_angle >= 180) s_angle = 180;
      
                s_s.write(s_angle);  

                
                
              }
            if (c.equals("fup")){
      
                s_angle -= 40;
      
                if (s_angle <= 0) s_angle = 0;
      
                s_s.write(s_angle);

              
                   
                }
            if (c.equals("fleft")){
      
              h_angle += 40;
      
              if (h_angle >= 180) h_angle = 180;
              
                s_h.write(h_angle);
                   
                }
            if (c.equals("fright")){
      
               h_angle -= 40;
              if (h_angle <= 0) h_angle = 0;
                s_h.write(h_angle);
                   
                }
      
          
      
      }        
      
      }
