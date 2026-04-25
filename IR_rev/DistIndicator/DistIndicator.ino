
int leds[3] = {11,12,13};

int ldr = A0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
for(int i=0;i<3;i++){

          pinMode(leds[i],OUTPUT);
      
      }

}

void loop() {
  // put your main code here, to run repeatedly:


int val = analogRead(A0);

val = map(val,0,1023,0,255);
      if (val < 65){
        
        digitalWrite(leds[0],HIGH);
                digitalWrite(leds[1],LOW);
        digitalWrite(leds[2],LOW);

        }

        else if (val >65 && val < 120){
        
        digitalWrite(leds[1],HIGH);
                digitalWrite(leds[0],LOW);
        digitalWrite(leds[2],LOW);

        }
        else {
          
                  digitalWrite(leds[2],HIGH);
                          digitalWrite(leds[1],LOW);
                                  digitalWrite(leds[0],LOW);



          }





delay(500);


Serial.println(val);


}
