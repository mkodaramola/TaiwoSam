char c;
int i = 0;

String s = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}


void loop() {
  // put your main code here, to run repeatedly:

  if(Serial.available()>0){
    
      c = Serial.read();


      if (c == 'g'){

          s = String(i)+ "|"+String(sin(i));
          Serial.println(s);
          i++;
        
        }
    
    }

}
