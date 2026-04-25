boolean s1,s2,s3,inits1,inits2,inits3;
byte noc = 0;
long pMillis = 0;
int arr[150][4];
float duration=0;
boolean Rec = false;
boolean t = false;

// Start Recording Function
void startRec(){

  Rec =true;

  if(t == false){
    pMillis = millis();
     t = true;
     Serial.println("START RECORDING");
  }

  noc = 0;
}

// Stop Recording Function
void stopRec(){
  Rec = false;
  t = false;
  Serial.println("STOP RECORDING");
}




String cmd = "";
boolean b = true;
byte trigger = 3;

// getVal Function

void getVal(){
    while(Serial.available()>0){
      
      if(b == true){
          cmd = "";
        } 
      
      cmd+= (char)Serial.read();
      
     b = false;
    }

    b = true;

    trigger = cmd.toInt();
  
}


// Onchange Function

boolean onChange(){
  if(Rec == true){

      s1 = digitalRead(8); s2 = digitalRead(12); s3 = digitalRead(13);
      if(s1 != inits1 || s2 != inits2 || s3 != inits3){
        delay(10);
        s1 = digitalRead(8); s2 = digitalRead(12); s3 = digitalRead(13);
      }

  if(s1 != inits1 || s2 != inits2 || s3 != inits3){

      
      duration = millis()-pMillis;

    
      arr[noc][0] = inits1;
      arr[noc][1] = inits2;
      arr[noc][2] = inits3;
      arr[noc][3] = duration;
        
     pMillis = millis();
      
      inits1 = s1; inits2 = s2; inits3 = s3;
      noc++;
      return true;
    }

    else{
      return false;
    }
    
    }

    else{
      return false;
    }
 
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  pinMode(8,INPUT);
  pinMode(12,INPUT);
  pinMode(13,INPUT);
  s1=0; s2=0; s3=0;
  inits1 = s1; inits2 = s2; inits3 = s3;


}

void loop() {
  // put your main code here, to run repeatedly:
    getVal();
  
  if(trigger == 1){
    startRec();
    
    trigger = 3;
  }
  if(trigger == 2) {

    stopRec();
   
    trigger = 3;
  }

  if(trigger == 5){
      play();
    }

  if(onChange() == true && Rec == true) {

    Serial.println("Change Occurred!");
    Serial.print(arr[noc-1][0]); Serial.print(" ");
    Serial.print(arr[noc-1][1]); Serial.print(" (");
    //Serial.print(arr[noc-1][2]); Serial.print(" ");
    Serial.print(arr[noc-1][3]/1000); Serial.println(" Secs) ");
    Serial.println(noc);
    
  }

}


void play(){
Serial.println("Playing... ");
int sum = 0;
for(int i=0;i<noc;i++){
  sum+=arr[i][3];
}
Serial.print("Time: "); Serial.print(sum/1000); Serial.println("Secs");

pinMode(8,OUTPUT);
pinMode(12,OUTPUT);
 
 for(int p=0;p<noc;p++){
  digitalWrite(8,arr[p][0]); digitalWrite(12,arr[p][1]);
  delay(arr[p][3]);
 }
 

 pinMode(8,INPUT);
 pinMode(12,INPUT);

Serial.println("Done!");
  
  }
