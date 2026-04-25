void setup() {
  // put your setup code here, to run once:
pinMode(13,OUTPUT);
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

digitalWrite(13,HIGH);
MyDelay(1000);
  Serial.println("1st Finish ");
digitalWrite(13,LOW);
MyDelay(1000);



}

long prev = 0;
void MyDelay(int interval){

    while(millis() - prev <= interval){
            if(millis() - prev >= (interval-5)){
              prev = millis();
      Serial.println("Prev");
              
              }
      Serial.print("Inside While  ");Serial.println(millis());
      }
  
  }
