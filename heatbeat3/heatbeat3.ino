#include <SoftwareSerial.h>
SoftwareSerial BTserial(2,3);
//String cmd= "";
//char sv = '0';
//char psv = '0';
//boolean f = true;
char cmd;
void setup(){
  
    //Initialize Serial Monitor
    Serial.begin(9600);
    //Initialize Bluetooth Serial Port
    BTserial.begin(9600);
    pinMode(12,OUTPUT);
}
void loop (){
    //Read data from HC06
//    while (BTserial.available()> 0){
//     
//      cmd+= (char)BTserial.read();
//
//    }

       //if(!cmd.equals("")) Serial.println(cmd);


      if(BTserial.available()>0){

              
   
     cmd = BTserial.read();

  
        
        }
        if(cmd != '!') Serial.println(cmd);

          if(cmd == 'a'){ digitalWrite(12,HIGH); Serial.println(cmd);}
    else if(cmd == 'b'){ digitalWrite(12,LOW); Serial.println(cmd);}
    
    cmd = '!';
    
     delay (500);

 

    //Write sensor data to HC06
//   if(f){
//    BTserial.print(sv);
//    f=false;
//    }
//
//    if(sv !=psv) {
//      
//      BTserial.print(sv);
//      psv = sv;
//    }
    //

   
}
