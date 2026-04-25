#include <TimerOne.h>
unsigned long f,k=512;// default 1000 μs (1000 Hz), meander, pulse 
int freq = 50;
unsigned long t = (1000000/freq);
byte k1,kn,kn1,kn2;
int drive,drive0;

void setup()
{ 
  pinMode(9, OUTPUT);
  pinMode(6,INPUT);// button at input 6
  pinMode(7,INPUT);// button at input 7 
  pinMode(13,INPUT);// button at input 13
  Serial.begin(9600);
}
void loop()
{
  Timer1.initialize(t); // period    
  Timer1.pwm(9, k); // k - fill factor 0-1023. 
  kn=digitalRead(6);// button input 6 (- pulse period) 
  kn1=digitalRead(7);// button input 7 (+ pulse period)
  kn2=digitalRead(13);// button input 13 (+ circle fill factor)

  if(kn==HIGH){ // decreasing the period (Increase Frequency)
    
    drive++;
    if(drive<30){ 
      t=t-1;  
    }
    // if the button is held for a long time, the correction of the pulse 
    else if(drive>30 && drive<60 ){ 
      t=t-10; 
    }
    else if(drive>=60 && drive<100){
      t=t-100;
    }
    else if(drive>=100){
      t=t-1000;
    }
  }
  else{
    drive=0;
  }

  if(kn1==HIGH){// adding a period (Decrease Frequency)
    drive0++;
    if(drive0<30){
      t=t+1; 
      // if the button is held for a long time, the correction of the 
    }
    else if(drive0>30 && drive0<60 ){ 
      t=t+10; 
    }
    else if(drive0>=60 && drive0<100){
      t=t+100;
    }
    else if(drive0>=100){
      t=t+1000;
    }
  } 
  else{
    drive0=0;
  }
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
   
  Serial.print("F=");
  Serial.print(f);
  Serial.println(" Hz");
  delay(300);
 
}
