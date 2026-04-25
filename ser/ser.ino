
int right = 9, left = 5, up = 3, down = 6; 
int val = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(right,OUTPUT);
  pinMode(left,OUTPUT);
  pinMode(up,OUTPUT);
  pinMode(down,OUTPUT);
  
}

int x = 0;
int y = 0;
void loop() {
  // put your main code here, to run repeatedly:

  if(Serial.available()) {

      String c = Serial.readString();

      int p = c.indexOf('|');     

      x = c.substring(0,p).toInt();

      y = c.substring(p+1).toInt();
      
     
     


       analogWrite(right,x);
       analogWrite(left,y);
       

      
     
    }


}


void resp(bool r, bool l, bool u, bool d){

  digitalWrite(right,r);
  digitalWrite(left,l);
  digitalWrite(up,u);
  digitalWrite(down,d);
  
  
  }
