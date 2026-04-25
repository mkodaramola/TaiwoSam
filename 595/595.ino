int clearPin = 5; //SHR 10
int serialData = 6;// SHR 14
int shiftClock = 7;//SHR 11
int latchClock =8;//SHR 12

void setup() {
  // put your setup code here, to run once:

   pinMode(shiftClock,OUTPUT);
   pinMode(latchClock,OUTPUT);
   pinMode(serialData,OUTPUT);
   pinMode(clearPin,OUTPUT);

   pinMode(13,OUTPUT);
  digitalWrite(clearPin,LOW); //this clear shift register
  digitalWrite(clearPin,HIGH); // Disable clear pin
}

void loop() {
  // put your main code here, to run repeatedly:

  for(int ct=0;ct<256;ct++){

          digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output

          shiftOut(serialData, shiftClock,MSBFIRST,ct); // shift out the bits

          digitalWrite(latchClock,HIGH); //send the new bit to the output 

          delay(500);
          
    
  }

}
