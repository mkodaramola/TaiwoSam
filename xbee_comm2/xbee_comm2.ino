#include <SoftwareSerial.h>

SoftwareSerial mySerial(4,5); //Tx,Rx


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  mySerial.begin(9600);
  

}

void loop() {
  // put your main code here, to run repeatedly:

    int v = analogRead(A0);
    v = v * 0.48828125;
    Serial.println(v);

    int ch = 0x3FB + v;

    byte checksum = ch;

    checksum = 0xFF - checksum;

    byte data[] = {0x7E,0x00,0x0F,0x10,0x01,0x00,0x41,0x9B,0x54,0x08,0xFF,0xFE,0x00,0x00,v,checksum};

    mySerial.write(data,19);


}
