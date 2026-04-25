#include <Wire.h>
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F,20,4);
IRrecv ir(8);

decode_results res;
int chan= 0;
boolean tst = false;

void setup() {
  // put your setup code here, to run once:
ir.enableIRIn();
Serial.begin(9600);
lcd.init();
lcd.begin(16,2);
lcd.backlight();
lcd.clear();

}

String text = "";
void loop() {
  // put your main code here, to run repeatedly:
  
 

  if(tst == false) password();

  if (tst == true) dis();

  
  
}


void password(){
int x=0;
int y=0;
  lcd.setCursor(0,0);
lcd.print("Enter Password: ");


if(ir.decode(&res)){
  
  Serial.println(res.value);

  long sig = res.value;

switch(sig){

   
   case 3862301447:
 text = text.substring(0,text.length()-1);
 lcd.clear();
  sig =0;
  break;

  
  case 3807251047:
  if (text.toInt() == 1234){
   tst = true;
    }
    else{
     incPass(); 
     delay(3000);

      }
      
  sig =0;
  text = "";
 break;
 
  
  case 3798652455:
   text += "1";
  sig =0;
  break;
  
  case 3545239723:
    text += "2"; 
    sig=0;
    break;
    
   case 1588114375:
    text += "3";
    sig = 0;
    break;
    
    case 1016816235:
    text += "4";
  sig =0;
  break;
  
  case 3678515779:
  text +="5";
  sig =0;
  break;
  
  case 4186844363:
  text += "6";
  sig =0;
  break;
  
  case 738462983:
  text += "7";
  sig =0;
  break;
  
  case 1927231079:
  text += "8";
  sig =0;
  break;
  
  case 2434700291:
  text += "9";
  sig =0;
  break;
  
  case 1097841703:
  text += "0";
  sig =0;
  break;

  case 1071866635:
  text = "";
  lcd.clear();
  sig =0;
  break;
 
  case 1228432235:
   //up
    if (y == 1) y -= 1;
  
  break;

   case 2637714691:
   //down
      if (y == 0) y += 1;

  sig =0;
  break;

   case 701051847:
   // right
   if (x < 15) x += 1;
   
  sig =0;
  break;

   case 2205584331:
   if (x > 0) x -= 1;
  sig =0;
  break;
  

  
  
  default: 
  ;
 
  }
 

   ir.resume();
  }


lcd.setCursor(0,1);
lcd.print(text);



  
  }

  

void dis(){
 lcd.clear();
      lcd.setCursor(0,0);
  lcd.print("Welcome!!!");

  if(ir.decode(&res)){
  
  Serial.println(res.value);

  long sig = res.value;

switch(sig){
  case 3358582699:
   tst = false;
  sig =0;
  break;
 
  default: 
  ;
 
  }
 

   ir.resume();
  }

  
  }

  void incPass(){
 lcd.clear(); 
 lcd.setCursor(0,0);
 lcd.print("Invalid Password!");
 delay(2000);
 lcd.clear();

  }
