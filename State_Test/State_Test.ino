float alt = 0.0;
int arr[2]={0,0};
unsigned long prev = 0;

String CSstate(float v){

   if(millis() - prev >= 1000){
        
        arr[0] = arr[1];
        arr[1] = int(v);

        prev = millis();

      }

  if(arr[1]-arr[0]> 10) return "ASCENT";

        else if(arr[1]-arr[0]<-10) return "DESCENT";

        else{ 
          
          if(v < 500)
          return "LAUNCH WAIT";

          else return "APOGEE";

        }
  
}


void setup() {
  
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
   alt = analogRead(A0);

   Serial.println(alt);
   
   Serial.println(CSstate(alt));
     
delay(100);
}
