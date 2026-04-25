//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#define Man 1
#define Bullet 2

IRrecv ir(4);

decode_results res;
byte lv = 3;
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

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
// -------------- Setup ------------------

void setup(){

  pinMode(13,OUTPUT);
  ir.enableIRIn();
  lcd.init();               
  lcd.begin(16,2);
  lcd.backlight();
  lcd.setCursor(0,1);
  lcd.write(1);
  lcd.clear();
  Serial.begin(9600);

   lcd.createChar(3,Introbullet);
   lcd.createChar(2,bullet);
   lcd.createChar(1,man);
lcd.setCursor(2,0);
lcd.write(3);
lcd.print(" BULLET 2D ");
lcd.write(Bullet);
delay(3000);
lcd.clear();
int perc = 0;
for(int i=0;i<16;i++){
lcd.setCursor(i-1,1);
lcd.write(3);
lcd.setCursor(0,0);
lcd.print(i*100/15);
lcd.print("%");
delay(150);
  }
delay(3000);
lcd.clear();
   

}


// -------------- Loop ------------


void loop(){

  if(lv == 4){
    Help();
    }
  
 if(lv == 3){
  Intro();
  } 
if(lv == 1){

  if(ft == true){
  lcd.clear();
  ft = false;
  }
  Game();
  }
 if(lv == 2){
  GameOver();
  }
  
  }

   // ------------ Intro ------------

  void Intro(){
  
  
    lcd.setCursor(0,irol);
    lcd.write(3);

    lcd.setCursor(1,0);
    lcd.print("Start Game");

    lcd.setCursor(0,irol);
    lcd.write(3);

    lcd.setCursor(1,1);
    lcd.print("Help");

         if(ir.decode(&res)){
  
  Serial.println(res.value);
  long sig = res.value;
switch(sig){
  case 3807251047:
    if (irol == 0){
       lv = 1;
   ft = true;
      }
       if (irol == 1){
        lcd.clear();
       lv = 4;
       
      }  
  break;
  case 1228432235:
   irol = 0;
    lcd.setCursor(0,1);
    lcd.print(" ");
  break;
  case 2637714691:
   irol = 1;
   lcd.setCursor(0,0);
    lcd.print(" ");
  break;
 
  default: 
  ;
 
  }
   ir.resume();
  }
     
    }

  //---------------------- Help ----------------
    void Help(){
  lcd.setCursor(0,0);
  lcd.print("Avoid bullet");
  lcd.setCursor(0,1);
  lcd.print("Use up/down key");
  if(ir.decode(&res)){
  
  Serial.println(res.value);

  long sig = res.value;

switch(sig){
  case 3862301447:
  lcd.clear();
   lv = 3;
  break;
 
  default: 
  ;
 
  }
 

   ir.resume();
  }
     
  
  }

 

  // ------------- Game --------------
 
  void Game(){
        digitalWrite(13,LOW);
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

    
if(ir.decode(&res)){
    
  long sig = res.value;

switch(sig){

  case 1228432235:
   //up
    rol = 0;
    sig = 0;
    lcd.setCursor(0,1);
      lcd.print(" ");
  break;

   case 2637714691:
   //down
   rol = 1;
      lcd.setCursor(0,0);
      lcd.print(" ");
      sig =0;
  break;

  case 3862301447:
  lcd.clear();
   lv = 3;
  break;
  
  default: 
  ;
  }
 
   ir.resume();
  }
    
    }
    
    // -------Game Over ---------------

void GameOver(){
    
    
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Score: ");
      lcd.print(score/(2*36));
      lcd.setCursor(k,1);
      lcd.print("Game Over!!!");
      if (k > 0){
        lcd.setCursor(k-1,1);
      lcd.print(" ");
        }
        digitalWrite(13,HIGH);
      delay(1000);

      k++;
      if(k > 15) k = 0;

       if(ir.decode(&res)){
  
  Serial.println(res.value);

  long sig = res.value;

switch(sig){
  case 3862301447:
   lv = 1;
   ft = true;
   score = 0;
   sig = 0;
   p = 0;
  break;
 
  default: 
  ;
 
  }
 

   ir.resume();
  }
    
    }
