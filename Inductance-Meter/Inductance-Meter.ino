
int struck = 4;

int sig = 10;

unsigned long pulse;

double freq,ind;

void setup() {
  // put your setup code here, to run once:

Serial.begin(115200);
pinMode(struck,OUTPUT);
pinMode(sig,INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:

  digitalWrite(struck,HIGH);
  delay(5);
  digitalWrite(struck,LOW);
  delayMicroseconds(100);

  pulse = pulseIn(sig,HIGH,5000);
  double cap = 1.E-6;
    freq = 2*((float)pulse);
    freq = 1.E6/freq;
    //Serial.print("Freq: "); Serial.println(freq);
    //Serial.print("Cap: "); Serial.println(cap);

    ind = 1./(4*3.14159*3.14159*freq*freq*cap);
 Serial.print("Pulse: "); Serial.println(pulse);
  if(pulse > 0.1){

    ind *= 1E6;
    Serial.print("Inductance: "); Serial.print(ind); Serial.println("uF");
    delay(700);
          
    }
    

}
