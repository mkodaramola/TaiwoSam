  
byte chargePin = 13;
byte dischargePin=12;
int Vin = A0;
boolean tst1 = true;
boolean tst2 = true;
unsigned long Btime=0;
unsigned long Etime=0;
double cap=0;
double Time=0;
void setup() {
  // put your setup code here, to run once:
pinMode(chargePin,OUTPUT);
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
//Serial.println(analogRead(A0));
digitalWrite(chargePin,HIGH);

  if(tst1 == true){
  Btime = millis();
  tst1 = false;
  }
  if(analogRead(A0)<375){

    
     if(tst2 == true){
    Etime = millis();
    tst2 = false;
    }
    
  Time = Etime - Btime;
   cap = (Time/10000*1000)+(0.07*(Time/10000*1000));
  
//    Serial.print("Btime: "); Serial.println(Btime);
//    Serial.print("Etime: "); Serial.println(Etime);
//    Serial.print("Time: "); Serial.println(Time);

    Serial.print("Capacitance: "); Serial.println(cap);

      digitalWrite(chargePin,LOW);
  pinMode(dischargePin,OUTPUT);
  digitalWrite(dischargePin,LOW);
  while(analogRead(A0)>0);
  pinMode(dischargePin,INPUT);
  

  
    tst1 = true;
    tst2 = true;
  delay(200);    
    }



}
