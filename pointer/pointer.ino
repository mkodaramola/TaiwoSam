boolean s1,s2,inits1,inits2;
int k = 1;
byte noc = 0;
int * p = new int[k];

int * Tp = NULL;
byte trigger = 3;

void setup() {
  // put your setup code here, to run once:

  
  Serial.begin(9600);
  pinMode(8,INPUT);
  pinMode(12,INPUT);
  s1=0; s2=0;
  inits1 = s1; inits2 = s2;

}

void loop() {
  // put your main code here, to run repeatedly:
  getVal();
  
  onChange();

  if(trigger == 1){
    
    for(int i=0;i<noc;i++){

        Serial.println(p[i]);
      }
    }
   
}

String cmd = "";
boolean b = true;


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


boolean onChange(){
      s1 = digitalRead(8); s2 = digitalRead(12);

  if(s1 != inits1 || s2 != inits2){

      
   
//      arr[noc][0] = inits1;
//      arr[noc][1] = inits2;

        if(k == 1) {
            *p = s1;
          }
         else{

            Tp = p;
            delete []p;
            p= new int[k];

            for(int i=0;i<(k-2);i++) *(p+i) = *(Tp + i);

            delete []Tp;

            *(p+k-1) = s1;
              
   
          
          }
        



  Serial.println("Change Happened");
      inits1 = s1; inits2 = s2;
      noc++;
      k++;
      return true;
    }

    else{
      return false;
    }

    
}
