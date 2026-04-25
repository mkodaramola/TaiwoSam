byte up = 4,enter=5,left=6,right=7,down=8,back=9;

void setup() {
  // put your setup code here, to run once:

      pinMode(up,INPUT);
      pinMode(down,INPUT);
      pinMode(right,INPUT);
      pinMode(left,INPUT);
      pinMode(enter,INPUT);
      pinMode(back,INPUT);
      Serial.begin(9600);
      
}

void loop() {
  // put your main code here, to run repeatedly:



      if(digitalRead(up) == HIGH){delay(400); Serial.println("Up");}

      if(digitalRead(down) == HIGH){delay(400); Serial.println("Down");}

      if(digitalRead(left) == HIGH) {delay(400);Serial.println("Left");}

      if(digitalRead(right) == HIGH){delay(400); Serial.println("Right");}

      if(digitalRead(enter) == HIGH) {delay(400);Serial.println("Enter");}

      if(digitalRead(back) == HIGH) {delay(400);Serial.println("Back");}


    


}
