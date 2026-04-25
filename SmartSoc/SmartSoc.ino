#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>
#define Man 1
#define Bullet 2
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

byte lv = 3;

BluetoothSerial SerialBT;


byte Introbullet[8] = {
  
 0b11000,
 0b11100,
 0b11110,
 0b11111,
 0b11111,
 0b11110,
 0b11100,
 0b11000  
  
  };

 byte man[8] = {
  
 0b00100,
 0b01010,
 0b00100,
 0b01110,
 0b10101,
 0b00100,
 0b01010  
  
  };

  
byte bullet[8] = {
  
 0b00011,
 0b00111,
 0b01111,
 0b11111,
 0b11111,
 0b01111,
 0b00111,
 0b00011  
  
  };


const int up = 34,enter=32,left=36,right=39,down=35,back=33, buzzer = 26, relay = 18;
byte hr=0,mins=0,sec=0;
byte crs = 0;
byte crsPos = 0;
String t = String(hr)+":"+String(mins)+":"+String(sec);
boolean once2 = false;
long dcdCT = 0;
byte page = 0;
boolean gst = false,once1 = false, Pause = false, alert = false;
long startTime = 0, timeSpent = 0, duration = 0;
String rTime = "";
byte alertD = 0; 

int col = 0;
int rol = 0;
int irol=0;
int i=0;
int ch = 0;
 int k = 0;
boolean ft = false;
int rad; 
int j = 0;
boolean tr = true;   
long score = 0;
float p = 0;
bool nextP = true;

LiquidCrystal_I2C lcd(0x27,16,2);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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
  delay(500);
  digitalWrite(buzzer,LOW);
  Mprint("SmartSoc",3,0);
  SerialBT.begin("SmartSoc");
  lcd.createChar(0,Introbullet);
  lcd.createChar(1,man);
   lcd.createChar(2,bullet);
   delay(500);
   lcd.clear();

  

}

void loop() {
  // put your main code here, to run repeatedly:

  bluetoothFunc();


  if(nextP){
      lcd.clear();
      nextP =false;
      }

   if (page == 0) select1();
   else if (page == 1) setTime();
   else if (page == 2) countDown();
   else if (page == 3) PauseFunc();
   else if (page == 4) routine();
   else if (page == 6) bullet2D();

   if (alert && alertD <= 10){

          digitalWrite (buzzer,HIGH);
          delay(500);
          digitalWrite (buzzer,LOW);
          delay(500);
          
          alertD++;
      
    }

   if (alertD >= 10){
    alert = false;
    alertD = 0;
    }

  delay(50);
}

byte br = 0;
byte cbr = 0;
void select1(){
  lcd.noBlink();
  control0();
  cbr = br;
  if(br == 2) cbr = 0;
  else if (br == 3) cbr = 1;
  lcd.setCursor(0,cbr);
  lcd.write(0);
  
  if (br < 2){
    Mprint("Timer",1,0);
  Mprint("Routine",1,1);
    }

   else if (br > 1){
    Mprint("Switch",1,0);
  Mprint("Game",1,1);
    }
  
  
  }

 

 void control0(){
  
  if(digitalRead(down) == LOW){
    delay(65);

    br += 1;
    if (br > 3) br = 0;
    nextP = true;
    }

    else if(digitalRead(up) == LOW){
    delay(65);

    br -= 1;
     if (br < 0) br = 3;
     nextP = true;
    }

     else if(digitalRead(enter) == LOW && br == 0){
    
    delay(65);
    page = 1;
    nextP = true;
    }

    else if(digitalRead(enter) == LOW && br == 1){
    
    delay(65);
    page = 4;
    nextP = true;
    }

    else if(digitalRead(enter) == LOW && br == 2){
    delay(65);
    page = 5;
    nextP = true;
    }

    else if(digitalRead(enter) == LOW && br == 3){
    delay(65);
    nextP = true;
    page = 6;
    
    }


     if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("u")) {
            br -= 1;
     if (br < 0) br = 3;
     nextP = true;  
        
    } 

    else if(cmd2.equals("d")) {
            br += 1;
    if (br > 3) br = 0;
    nextP = true;
    }
    else if(cmd2.equals("e") && br == 0) {
            page = 1;
    nextP = true;
    }
    else if(cmd2.equals("e") && br == 1) {
            page = 4;
    nextP = true;
    }
    else if(cmd2.equals("e") && br == 2) {
             page = 5;
    nextP = true;
    }
    else if(cmd2.equals("e") && br == 3) {
         nextP = true;
    page = 6;   
    }
    
  }

    
  
  }

  bool wordPr(String w, String t){
    t.toLowerCase();
    if (t.indexOf(w) >= 0) return true;
    else return false;
       
    }

void bluetoothFunc(){
  
  if (SerialBT.available()) {
    String cmd = SerialBT.readString();
    
    if (cmd.equals("1") || (wordPr(" on ",cmd) && (wordPr("switch",cmd) || wordPr("turn",cmd) || wordPr("put",cmd)))) {
      digitalWrite(relay, HIGH); // Turn on the relay
      Serial.println("Relay ON");
    } else if (cmd.equals("0") || (wordPr(" off ",cmd) && (wordPr("switch",cmd) || wordPr("turn",cmd) || wordPr("put",cmd)))) {
      digitalWrite(relay, LOW); // Turn off the relay
      Serial.println("Relay OFF");
    }
  }

  delay(20);
  
  }







void routine(){
  Mprint("Set Routine",0,1);

  if (digitalRead(back) == LOW){
            delay(65);
            page = 0;
        
        }

        
        if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("b")) {
            
        page = 0;
    } 
    
  }
  
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
   


      if(digitalRead(back) == LOW) {
        delay(200);
        Serial.println("back");
        alert = false;
        gst = false;
        Pause = false;
        once1 = false;
        alertD = 0;
        hr = 0; mins = 0; sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
        timeSpent = 0;
       duration = 0;
       lcd.clear();
        page = 1; 
        digitalWrite(relay, LOW);   

           nextP = true;
        
        } 


     else if(digitalRead(enter) == LOW) {
      delay(300);
        Serial.println("enter");

            Pause = true;

            if (Pause){

              duration = duration - timeSpent;

              once1 = true;
              gst = false;
              page = 3;

              lcd.clear();
              nextP = true;
              }
        
        
        } 


        if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("e")) {
               Pause = true;

            if (Pause){

              duration = duration - timeSpent;

              once1 = true;
              gst = false;
              page = 3;

              lcd.clear();
              nextP = true;
              }
        
    } 

    else if(cmd2.equals("b")) {
              alert = false;
        gst = false;
        Pause = false;
        once1 = false;
        alertD = 0;
        hr = 0; mins = 0; sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
        timeSpent = 0;
       duration = 0;
       lcd.clear();
        page = 1; 
        digitalWrite(relay, LOW);   

           nextP = true;
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

  
  Mprint("PAUSED",3,0);
  Mprint(S_HMS(duration),4,1);
    
    if(digitalRead(enter) == LOW && page == 3) {
      
      delay(300);

          Serial.println("enter");

              Pause = false;
              lcd.clear();
              page = 2;
            nextP = true;
        }


 if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("e") && page == 3) {
              Pause = false;
              lcd.clear();
              page = 2;
            nextP = true;
    } 
    
  }
    
  

  
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
     
      if (digitalRead(up) == LOW && hr < 30 && crs == 2){
        delay(65);
        nextP = true;
        Serial.println("Up"); 
        hr+=1;
        if (hr > 30) hr = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
        
      }
    else if (digitalRead(up) == LOW && mins < 59 && crs == 1){
      delay(65);
      nextP = true;
      Serial.println("Up");
        mins +=1;
        if (mins > 59) mins = 0;
         t = String(hr)+":"+String(mins)+":"+String(sec);
         
      }
    else if (digitalRead(up) == LOW && sec < 59 && crs == 0){
      delay(65);
      nextP = true;
      Serial.println("Up");
        sec +=1;
        if (sec > 59) sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
        
    }
//--- down
    else if (digitalRead(down) == LOW && hr < 30 && crs == 2 && hr !=0){ 
        delay(65);
        nextP = true;
        Serial.println("down");
        hr-=1;
        if (hr < 0) hr = 29;
         t = String(hr)+":"+String(mins)+":"+String(sec);
         
      }
    else if (digitalRead(down) == LOW && mins < 59 && crs == 1 && mins != 0){
        delay(65);
        nextP = true;
                Serial.println("down");

        mins -=1;
        if (mins < 0) mins = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
         
      }
    else if (digitalRead(down) == LOW && sec < 59 && crs == 0 && sec != 0){
        delay(65);
        nextP = true;
                Serial.println("down");

        sec -=1;
        if (sec < 0) sec = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      }

// -- left/right
    else if (digitalRead(left) == LOW){
          delay(65);
          nextP = true;;
                  Serial.println("left");

          crs +=1;
          if (crs > 2) crs = 0;
           t = String(hr)+":"+String(mins)+":"+String(sec);

      }  


     else if (digitalRead(right) == LOW){
          delay(65);
          nextP = true;
                  Serial.println("right");

          crs -=1;
          if (crs < 0) crs = 2;
           t = String(hr)+":"+String(mins)+":"+String(sec); 
      }
// -- enter
      else if (digitalRead(enter) == LOW && page == 1){
               delay(65);
               nextP = true;
                       Serial.println("enter");

              duration = HMS_S(hr,mins,sec)*1000;

              page = 2;
          
        }


       else if (digitalRead(enter) == LOW){
            delay(65);
            nextP = true;
                    Serial.println("enter");

            hr = 0; mins = 0; sec =0;
            t = String(hr)+":"+String(mins)+":"+String(sec);
            page = 1;
        
        }

        else if (digitalRead(back) == LOW){
            delay(70);
            nextP = true;
             alertD = 0;
            page = 0;
            br = 0;
            
        }



        if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("u") && sec < 59 && crs == 0) {
            nextP = true;
      Serial.println("Up");
        sec +=1;
        if (sec > 59) sec = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
    } 
    else if (cmd2.equals("u") && hr < 30 && crs == 2) {
      nextP = true;
        Serial.println("Up"); 
        hr+=1;
        if (hr > 30) hr = 0;
        t = String(hr)+":"+String(mins)+":"+String(sec);
    }
     else if (cmd2.equals("u") && mins < 59 && crs == 1) {
      nextP = true;
      Serial.println("Up");
        mins +=1;
        if (mins > 59) mins = 0;
         t = String(hr)+":"+String(mins)+":"+String(sec);
    }
     else if (cmd2.equals("d") && hr < 30 && crs == 2 && hr !=0) {
       nextP = true;
        Serial.println("down");
        hr-=1;
        if (hr < 0) hr = 29;
         t = String(hr)+":"+String(mins)+":"+String(sec);
    }

    else if (cmd2.equals("d") && mins < 59 && crs == 1 && mins != 0) {
      nextP = true;
                Serial.println("down");

        mins -=1;
        if (mins < 0) mins = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
    }

    


    else if (cmd2.equals("d") && sec < 59 && crs == 0 && sec != 0) {

      nextP = true;
      Serial.println("down");

        sec -=1;
        if (sec < 0) sec = 59;
         t = String(hr)+":"+String(mins)+":"+String(sec);
      
    }

    else if (cmd2.equals("l")) {
      nextP = true;;
                  Serial.println("left");

          crs +=1;
          if (crs > 2) crs = 0;
           t = String(hr)+":"+String(mins)+":"+String(sec);
    }

    else if (cmd2.equals("r")) {
      nextP = true;
                  Serial.println("right");

          crs -=1;
          if (crs < 0) crs = 2;
           t = String(hr)+":"+String(mins)+":"+String(sec);
    }

    else if (cmd2.equals("e") && page == 1) {
      nextP = true;
      Serial.println("enter");

       duration = HMS_S(hr,mins,sec)*1000;

        page = 2;
    }

    else if (cmd2.equals("e")) {

      nextP = true;
                    Serial.println("enter");

            hr = 0; mins = 0; sec =0;
            t = String(hr)+":"+String(mins)+":"+String(sec);
            page = 1;
      
    }

    else if (cmd2.equals("b")) {
       nextP = true;
       alertD = 0;
      page = 0;
      br = 0;
    }

    
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




  void bullet2D(){

    if(lv == 1){
        if(ft == true){
        lcd.clear();
        ft = false;
        }
      
     Game();
    }
      
    else if(lv == 2) GameOver();
    
    else if(lv == 3) Intro();

    else if(lv == 4) Help();
    

    }


    // ------------ Intro ------------

  void Intro(){
    
      
    lcd.setCursor(1,0);
    lcd.print("Start Game");

    lcd.setCursor(0,irol);
    lcd.write(0);

    lcd.setCursor(1,1);
    lcd.print("Help");

    

    if(digitalRead(enter) == LOW){
      if (irol == 0){
       lv = 1;
      ft = true;
      }
       else if (irol == 1){
        lv = 4;      
      }  

      nextP = true;
  }

if(digitalRead(up) == LOW){
      irol = 0;
    nextP = true;
  }

else if(digitalRead(down) == LOW){
      irol = 1;
    nextP = true;
  }

 
else if(digitalRead(back) == LOW){
      irol = 0;
      page = 0;
      br = 3;
    nextP = true;
  }

   if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("u")) {
         irol = 0;
    nextP = true;   
    } 
    else if (cmd2.equals("d")) {
      irol = 1;
    nextP = true;
    }
     else if (cmd2.equals("b")) {
      irol = 0;
      page = 0;
      br = 3;
    nextP = true;
    }
     else if (cmd2.equals("e")) {
      if (irol == 0){
       lv = 1;
      ft = true;
      }
       else if (irol == 1){
        lv = 4;      
      }  

      nextP = true;
    }
  }

  

    }

    
//  //---------------------- Help ----------------

    void Help(){
  lcd.setCursor(0,0);
  lcd.print("Avoid bullet");
  lcd.setCursor(0,1);
  lcd.print("Use up/down key");



if(digitalRead(back) == LOW){
      lv = 3;
    nextP = true;
  }

  if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("b")) {
       lv = 3;
    nextP = true;
    } 
    
  }

  

    
  }


//  // ------------- Game --------------

  void Game(){
        
        Serial.println(p);
        Serial.println(350-p);


  if (tr == true){ 
    rad = random(2);
    tr = false;
  }
   
  lcd.setCursor(col,rol);
  lcd.write(Man);

 if(rad == 0){
   lcd.setCursor(16-i,rad);
  lcd.write(Bullet);
  lcd.setCursor(16-i+1,0);
  lcd.print(" ");
  }
 if(rad == 1){
   lcd.setCursor(16-j,rad);
  lcd.write(Bullet);
  lcd.setCursor(16-j+1,1);
  lcd.print(" ");
  } 
  delay(250-p);
  p+=0.5;

  if (350-p)
  score += (p);

  if (p >= 200){
    p = 200;
    }
  
  if(i > 16){
    i=0;
    tr = true;
    }
    if (rad == 0){
      i++;
      }

   if(j > 16){
    j=0;
    
    tr = true;
    }
    if (rad == 1){
      j++;
      }
    

  if (i == 15 && rol == 0){

    lv = 2;    
    }
    if (j == 15 && rol == 1){

    lv = 2;    
    }




   if(digitalRead(up) == LOW){
       rol = 0;    
    lcd.setCursor(0,1);
      lcd.print(" ");
    nextP = true;
  }


   else if(digitalRead(down) == LOW){
       rol = 1;
      lcd.setCursor(0,0);
      lcd.print(" ");
    nextP = true;
  }

  else if(digitalRead(back) == LOW){
       lcd.clear();
      lv = 3;
    nextP = true;
  }

   if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("u")) {
       rol = 0;    
    lcd.setCursor(0,1);
      lcd.print(" ");
    nextP = true;   
    } 
    else if (cmd2.equals("d")) {
      rol = 1;
      lcd.setCursor(0,0);
      lcd.print(" ");
    nextP = true;
    }
  }

 


  
  
    
    }
    
//    // -------Game Over ---------------
int gocs = 0;
void GameOver(){
    
    
      if(gocs%1000 == 0) nextP = !nextP;
      lcd.setCursor(0,0);
      lcd.print("Score: ");
      lcd.print(score/(2*36));
      lcd.setCursor(k,1);
      lcd.print("Game Over!!!");
      if (k > 0){
        lcd.setCursor(k-1,1);
      lcd.print(" ");
        }
      

      k++;
      if(k > 15) k = 0;

 

  if(digitalRead(back) == LOW){
       lv = 1;
       ft = true;
       score = 0;
     
       p = 0;
    nextP = true;
  }

   if (SerialBT.available()) {
    String cmd2 = SerialBT.readString();
    
    if (cmd2.equals("b")) {
         lv = 1;
       ft = true;
       score = 0;
     
       p = 0;
    nextP = true;
    } 
    
  }

    gocs++;
    }
