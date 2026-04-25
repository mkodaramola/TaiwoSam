int Fval = 310;
int Sval = 0;
int Ival=9;
int val = 300;
int d=100;
void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
pinMode(Ival, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:

  switch (Serial.read()){
    case 's':
        val+= 100;
      break;

       case 'a':
       val-=100;
       break;  
    
    }

analogWrite(Ival,Fval);
Sval = analogRead(A0);

if (Sval < val){
  
  Fval+=2;
  }

if (Sval > val){
  Fval -= 2;
  }

 
Serial.print("First Motor------> "); Serial.print(Fval); Serial.print("  |  "); Serial.print(Sval);  Serial.println("  <----- Second Motor: ");
Serial.print("Friction: "); Serial.println(val); 



}
