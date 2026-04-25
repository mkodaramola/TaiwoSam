

String SRes="";

int R=0;
void setup() {
  // put your setup code here, to run once:

    Serial.begin(9600);
    
      Serial.println(2.E-12); 
}

//float val = analogRead(A2);
//float volt = (val*5)/470.5;
//VD = (val/1023)*volt;
//
//    volt = constrain(volt,0,9);
//    
//
//Res = (1000*VD)/(volt-VD);
//Res =(Res*1000)/(1000-Res);


void loop() {
  // put your main code here, to run repeatedly:

float Res = analogRead(A2);
Serial.println(Res);

float Rvolt = (Res*5)/470.5;
Serial.println((Res/1023)*Rvolt);
Rvolt = constrain(Rvolt,0,9);
float VD = (Res/1023)*Rvolt;

Res = (1000*VD)/(Rvolt-VD);
Res =(Res*1000)/(1000-Res);

//Res = (5000/Rvolt) - 1000; 
//Rvolt = 5000/(Res +1000);
//Res = (1000*Res)/(1000-Res);
if(Rvolt == 0){
   SRes = "Insert A Resistor";
  }
else if(Res >= 1000){
Res = Res/1000;

SRes = String(Res);
SRes += "K";

}
else if(Res < 1000) {
  
  SRes = String(Res);
  }

 

Serial.print("Voltage: "); Serial.print(Rvolt);
Serial.print("\tResistor: "); Serial.println(SRes);

delay(800);
}
