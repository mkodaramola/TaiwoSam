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

  if(var == '1'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,128); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);  
    
  }
  else if(var == '2'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,64); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);   
             
             }
     else if(var == '3'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,32); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
     else if(var == '4'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,16); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
     else if(var == '5'){
 
              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,8); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
     else if(var == '6'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,4); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
     else if(var == '7'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,2); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
     else if(var == '8'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,1); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }
       else if(var == '9'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,170); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }

           else if(var == '0'){

              digitalWrite(clearPin,LOW);
              digitalWrite(clearPin,HIGH);
    
            digitalWrite(latchClock,LOW); // No new bits should be transmitted to the output
            shiftOut(serialData, shiftClock,MSBFIRST,0); // shift out the bits
            digitalWrite(latchClock,HIGH); //send the new bit to the output 
            
             digitalWrite(clearPin,LOW);
             digitalWrite(clearPin,HIGH);
    }

  delay(500);

}
