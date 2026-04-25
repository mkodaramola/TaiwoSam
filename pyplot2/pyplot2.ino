char c;
int c_i = 0;
int t_i = 0;

float t = 23.3;
String s = "";

boolean started = false;
String mode = "ASCENT";
boolean Bmode = false;
long inM = 0; 
long t_inM = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  randomSeed(analogRead(A0));
}

void loop() {
  float GPS_TIME = random(0, 24);
  float GPS_LATITUDE = random(1223, 9000);
  float GPS_LONGITUDE = random(1223, 9000);
  float GPS_STATS = random(0,2);
  float GPS_ALTITUDE = c_i * 1.1;
//  float TEMP = t + 1.23;
 float TEMP = sin(t_i);
  int ALTITUDE = c_i * 2;

if (c_i%45 == 0) Bmode = !Bmode;

if(Bmode) mode = "ASCENT";
else mode = "DESCENT";

  String s = "C|";
  s.concat("2082,");
  s.concat("12:05,");
  s.concat(String(c_i));
  s.concat(",F,");
  s.concat(mode);
  s.concat(",");
  s.concat(String(ALTITUDE));
  s.concat(",");
  s.concat(String(TEMP*1.78));
  s.concat(",N,N,");
  s.concat(String(TEMP));
  s.concat(",");
  s.concat(String(GPS_LATITUDE));
  s.concat(",");
  s.concat("7.01");
  s.concat(",");
  s.concat(String(GPS_TIME));
  s.concat(",");
  s.concat(String(GPS_ALTITUDE));
  s.concat(",");
  s.concat(String(GPS_LATITUDE));
  s.concat(",");
  s.concat(String(GPS_LONGITUDE));
  s.concat(",");
  s.concat(String(GPS_STATS));
  s.concat(",");
  s.concat(String(TEMP+0.18));
  s.concat(",");
  s.concat(String(TEMP-0.54));
  s.concat(",");
  s.concat(String(TEMP+0.77));
  s.concat(",");
  s.concat("CXON");
  s.concat(",");
  s.concat(String(TEMP+0.98));
  
  
  
  
  s.concat(",NIL,NIL,");


  if(!started) {
      inM = millis();
      started = true;
    }

    if(millis() - inM >= 1000) {
      
      Serial.println(s);

       c_i++;
       t_i++;
      inM = millis();
    }

 

}
