void setup() {
  // put your setup code here, to run once:

   DDRB = B00100000;  
  PORTB = B00000000;
  Serial.begin(9600);
  
}

void loop() {
  // put your main code here, to run repeatedly:
//  delay(300);
//  PORTB = PORTB << 1;
//  Serial.println((PINB &(1<<4))>>4);
//  if ((PINB &(1<<4))>>4 == HIGH) PORTB=1;

PORTB = B00100000;
Serial.println(PORTB, BIN);
delay(350);
PORTB = PORTB >> 1;
Serial.println(PORTB, BIN);
delay(350);

  }
