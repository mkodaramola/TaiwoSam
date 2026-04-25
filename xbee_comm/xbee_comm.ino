#include <Printers.h>
#include <XBee.h>

XBee xbee = XBee();

uint8_t payload[] = {0,0,0,0,0,0,0};

XBeeAddress64 addr64 = XBeeAddress64(0x000000000,0x000000000);
ZBTxRequest tx = ZBTxRequest(addr64, payload, sizeof(payload));



void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  xbee.setSerial(Serial);

 
  

}

void loop() {
  // put your main code here, to run repeatedly:

  payload[0]='T';
  payload[0]='e';
  payload[0]='m';
  payload[0]='p';
  payload[0]='|';

 int v = analogRead(A0);

  payload[5] = v;
  payload[6] = v+36;

  xbee.send(tx);

 

}
