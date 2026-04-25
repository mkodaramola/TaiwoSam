#include <LiquidCrystal_I2C.h>

byte up = 4,enter=5,left=6,right=7,down=8,back=9, buzzer = 12, relay = 10;
byte hr=0,mins=0,sec=0;
byte crs = 0;
byte crsPos = 0;
String t = String(hr)+":"+String(mins)+":"+String(sec);
boolean once2 = false;
long dcdCT = 0;
byte page = 1;
boolean gst = false,once1 = false, Pause = false, alert = false;
long startTime = 0, timeSpent = 0, duration = 0;
String rTime = "";


LiquidCrystal_I2C lcd(0x27,16,2);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
    lcd.init();
  lcd.backlight();
  pinMode (buzzer,OUTPUT);
  pinMode(relay,OUTPUT);
  pinMode(up,INPUT);
  pinMode(down,INPUT);
  pinMode(right,INPUT);
  pinMode(left,INPUT);
  pinMode(enter,INPUT);
  pinMode(back,INPUT);
  digitalWrite(buzzer,HIGH);
  delay(3000);
   digitalWrite(buzzer,LOW);

  
}

void loop() {
  // put your main code here, to run repeatedly:

  
   
   if (page == 1) setTime();
   else if (page == 2) countDown();
   else if (page == 3) PauseFunc();

   
  
}



void countDown(){

  if (!gst){
    
      startTime = millis();
      gst = true;
    }

    timeSpent = millis() - startTime;
    digitalWrite(relay,HIGH);
   if (timeSpent >= duration){

        gst = false;
        Pause = false;
        alert = true;
        once1 = false;
        hr = 0; mins = 0; sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
       timeSpent = 0;
       duration = 0;
        lcd.clear();
        page = 1;
       
     digitalWrite(relay,LOW);
    
    }
   


      if(digitalRead(back) == HIGH) {
        delay(65);
                Serial.println("back");

        gst = false;
        Pause = false;
        once1 = false;
        hr = 0; mins = 0; sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
        timeSpent = 0;
       duration = 0;
       lcd.clear();
        page = 1;       
        
        } 


     else if(digitalRead(enter) == HIGH) {
      delay(65);
        Serial.println("enter");

            Pause = true;

            if (Pause){

              duration = duration - timeSpent;

              once1 = true;
              gst = false;
              page = 3;

              lcd.clear();
              
              }
        
        
        } 
        
    


      if (!once2){

        dcdCT = millis();

        once2 = true;
        
        }
        


    rTime = S_HMS(duration - timeSpent);
     
     if ((millis() -dcdCT) >= 1000) {
      lcd.clear();
      Mprint("Time left:",3,0);
      Mprint(rTime,4,1);

      once2 = false;

     }
  
  
  
  }


void PauseFunc(){

  lcd.clear();
  Mprint("PAUSED",3,0);
    Mprint(S_HMS(duration),4,1);

  
    
    
    if(digitalRead(enter) == HIGH && page == 3) {delay(65);

          Serial.println("enter");

              Pause = false;
              lcd.clear();
              page = 2;
          
        }
    
  

  
  }


void Alert(){ 
  digitalWrite (buzzer,HIGH);
}


void setTime(){

    control();
    Mprint("Set time",3,0);
    Mprint(t,4,1);
    
    if (crs == 0) Mcrs(3,t);
    else if (crs == 1) Mcrs(2,t);
    else Mcrs(1,t);

       
    delay(100);
  }

void Mcrs(byte p, String s){

      if (p == 1){      
        lcd.blink();
      lcd.setCursor(4,1);
        }
     else if (p == 2){
        int a =s.indexOf(':');
       lcd.blink();
      lcd.setCursor(4+a+1,1);
      }

     else{
      int a =s.lastIndexOf(':');
       lcd.blink();
      lcd.setCursor(4+a+1,1);
      
      }
    }

void control(){


        lcd.clear();
      if (digitalRead(up) == HIGH && hr < 30 && crs == 2){
        delay(65);
        Serial.println("Up"); 
        hr+=1;
        if (hr > 30) hr = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
      }
    else if (digitalRead(up) == HIGH && mins < 59 && crs == 1){
      delay(65);
      Serial.println("Up");
        mins +=1;
        if (mins > 59) mins = 0;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      }
    else if (digitalRead(up) == HIGH && sec < 59 && crs == 0){
      delay(65);
      Serial.println("Up");
        sec +=1;
        if (sec > 59) sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
    }

    else if (digitalRead(down) == HIGH && hr < 30 && crs == 2 && hr !=0){ 
        delay(65);
        Serial.println("down");
        hr-=1;
        if (hr < 0) hr = 29;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      }
    else if (digitalRead(down) == HIGH && mins < 59 && crs == 1 && mins != 0){
        delay(65);
                Serial.println("down");

        mins -=1;
        if (mins < 0) mins = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      }
    else if (digitalRead(down) == HIGH && sec < 59 && crs == 0 && sec != 0){
        delay(65);
                Serial.println("down");

        sec -=1;
        if (sec < 0) sec = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      }

    else if (digitalRead(left) == HIGH){
          delay(65);
                  Serial.println("left");

          crs +=1;
          if (crs > 2) crs = 0;
           t = String(hr)+":"+String(mins)+":"+String(sec);

      }  


     else if (digitalRead(right) == HIGH){
          delay(65);
                  Serial.println("right");

          crs -=1;
          if (crs < 0) crs = 2;
           t = String(hr)+":"+String(mins)+":"+String(sec); 
      }

      else if (digitalRead(enter) == HIGH && page == 1){
               delay(65);
                       Serial.println("enter");

              duration = HMS_S(hr,mins,sec)*1000;

              page = 2;
          
        }


       else if (digitalRead(enter) == HIGH){
            delay(65);
                    Serial.println("enter");

            hr = 0; mins = 0; sec =0;
            t = String(hr)+":"+String(mins)+":"+String(sec);
            page = 1;
        
        }

  }

long HMS_S(byte hr,byte mins, byte sec){

   int Tsec = 0;

   Tsec += sec;
   Tsec += (mins*60);
   Tsec += (hr*3600);
  
    return Tsec;
  }


String S_HMS(long s){

      s /= 1000;

     int iHr = s/3600;

     s -= (iHr*3600);

     int iMin = s/60;

     s -= (iMin*60);

     return String(iHr)+":"+String(iMin)+":"+String(s);
  
  }


void Mprint(String tx, byte c,byte r){
      lcd.setCursor(c,r);
      lcd.print(tx);      
    }

   
