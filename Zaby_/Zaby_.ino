#include <SoftwareSerial.h>

SoftwareSerial BTSerial (2,3);

int Zaby_SW = 13;
boolean Z_state = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (9600);
  pinMode (Zaby_SW, OUTPUT);

  Serial.println("Enter AT Commands: ");
  BTSerial.begin (9600);
  
}

String cmd= "";

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println(analogRead(A0));

  while (BTSerial.available()> 0){
      cmd+= (char)BTSerial.read();

    }

    Serial.print("#");
    Serial.print(cmd);
    Serial.println("#");

    if(analogRead(A0) <= 175){
      if(cmd.substring(0,1) == "a") Z_state = true;

      else if (cmd.substring(0,1) == "b") Z_state = false;

      digitalWrite(Zaby_SW,Z_state);
    }

    else {
      digitalWrite(Zaby_SW,LOW);
      }

     if (Serial.available ()){
      BTSerial.write(Serial.read());
     }

     }
     
  delay(100);
 cmd = "";
}
