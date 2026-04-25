
//TFT Display

class Window{

  
     

      
  };

void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600); 
  
}


String str="";
void loop() {
  // put your main code here, to run repeatedly:


  while(Serial.available()){
     
      str = Serial.readString();
      
}

Serial.print("Hello "); Serial.println(str);

}
