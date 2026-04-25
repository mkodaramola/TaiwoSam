int clearPin = 5; //SHR 10
int serialData = 10;// SHR 14
int shiftClock = 11;//SHR 11
int latchClock =12;//SHR 12
char var = '0';
void setup() {
  // put your setup code here, to run once:

   pinMode(shiftClock,OUTPUT);
   pinMode(latchClock,OUTPUT);
   pinMode(serialData,OUTPUT);
   pinMode(clearPin,OUTPUT);

   pinMode(13,OUTPUT);
  digitalWrite(clearPin,LOW); //this clear shift register
  digitalWrite(clearPin,HIGH); // Disable clear pin
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

   while(Serial.available() == 0){
   
    }

     var = Serial.read();



              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,var-'0'); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            Serial.println("Value"); Serial.println(var - '0');
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);  
    
  
  

  delay(500);

}
