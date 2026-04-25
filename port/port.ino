int timer = 1000;
void setup() {
  // put your setup code here, to run once:
DDRB |= B00100000;
PORTB = B00000000;
}

void loop() {
  // put your main code here, to run repeatedly:


 while (count <= 60){
  
   PORTB |= B0100000;
  delay(timer);
  PORTB &= B0000000;
  delay(timer);
  timer-=15; 
  count++;
  }
  
  
}


// while (count <= 60){
//   PORTB |= B0100000;
//  delay(1000);
//  PORTB &= B0000000;
//  delay(1000); 
//  count++;
//  }
