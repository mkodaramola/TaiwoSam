#include <TimerOne.h>
#include <FreqCount.h>
unsigned long f = 5,t,k=512;// default 1000 μs (1000 Hz), meander, pulse 
byte k1,kn,kn1,kn2;

void setup()
{ 
  pinMode(9, OUTPUT); 
 // pinMode(13,INPUT);// button at input 13
  Serial.begin(9600);
  FreqCount.begin(1000);
  pinMode(5,INPUT);
}

String cmd = "";
boolean b = true;

int getVal(){
    while(Serial.available()>0){
      
      if(b == true){
          cmd = "";
        } 
      
      cmd+= (char)Serial.read();
      
     b = false;
    }

    b = true;

    return cmd.toInt();
  
}

float count = 0;
void loop()
{

   if(getVal() == 0) t = 1000000/5;
   else t = 1000000/getVal();
   
  Timer1.initialize(t); // period    
  Timer1.pwm(9, k); // k - fill factor 0-1023. 
  kn=digitalRead(6);// button input 6 (- pulse period) 
  kn1=digitalRead(7);// button input 7 (+ pulse period)
  kn2=digitalRead(13);// button input 13 (+ circle fill factor)

 

  if(t==0 || t>300000){ // limiting the pulse duration to the minimum, if 
    t=1;
  }
  if(t>200000 && t<300000){ // limiting the pulse duration to the 

    t=200000;
  } 
  f=1000000/t; // calculate the frequency 
  k1=k*100/1024; // calculate% fill factor

  if(kn2==HIGH){// button for adjusting the fill factor (in a circle from 


    k=k+16;// step 16 out of 1024 (you can do 8 for smoother adjustment)
  }
  if(k==1024){
    k=0;
  }
 
//  Serial.print("T=");
//  Serial.print(t);
//  Serial.println(" us");
// 
//  Serial.print(k1);
//  Serial.println(" %");
   
  //Serial.print("Freq Gen=");
  Serial.print(f);
  Serial.print(" Hz       ");
  //delay(300);

    if (FreqCount.available()) {                        //if the code if working
     count = FreqCount.read();                   //create float var called count and populate it with current frequency count
    float period = (1/count);                         //create float var called period and populate it with the inverse of the frequency
                                 
                              //print units to buffer & drop down 1 line
//  Serial.print("Period: ");                          //print the name of the fuction to buffer
//  Serial.print(period*1000);                         //print the period of signal in milliseconds to buffer
//  Serial.println("mS");                                //print the units to buffer
  }

    //Serial.print("Freq Counted:   ");                          //print the name of the function to buffer
  Serial.print(count/65535);                               //print the actual counted frequency to buffer
  Serial.println("Hz"); 
 delay(300);
}
