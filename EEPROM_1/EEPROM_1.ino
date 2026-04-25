#include <EEPROM.h>

String v = "Jesus is Lord";
int val[5];

int arr[5];





void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

       for(int i=0;i<5;i++) arr[i] = i;

    EEPROM.put(22,arr);
     delay(10);


  Serial.println("DONE");

  

} 

void loop() {
  // put your main code here, to run repeatedly:
 

    EEPROM.get(22,val);
    Serial.println(val);
     delay(500);


 
}
