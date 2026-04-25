
 /* 
 * learnelectronics
 * 27 AUG 2017
 * 
 * www.youtube.com/c/learnelectronics
 * arduino0169@gmail.com
 * 
 * NOTES: Signal must be logic level
 *        Input pin is D5
 *        No analogWrite() on 3, 9, 1, 11
 */

                        //OLED driver
#include <FreqCount.h>                                //FreqCount library for you know...counting frequencies
                                                      //find it here: https://github.com/PaulStoffregen/FreqCount




void setup()   {                
   Serial.begin(9600);
   FreqCount.begin(1000);                              //start counting 1,2,3,4...
    pinMode(5,INPUT);
}


void loop() {

 
  if (FreqCount.available()) {                        //if the code if working
    float count = FreqCount.read();                   //create float var called count and populate it with current frequency count
    float period = (1/count);                         //create float var called period and populate it with the inverse of the frequency
                                 
  Serial.print("Freq:   ");                          //print the name of the function to buffer
  Serial.print(count);                               //print the actual counted frequency to buffer
  Serial.println("Hz");                              //print units to buffer & drop down 1 line
  Serial.print("Period: ");                          //print the name of the fuction to buffer
  Serial.print(period*1000);                         //print the period of signal in milliseconds to buffer
  Serial.println("mS");                                //print the units to buffer
  }
  
  
  
}                                                     
