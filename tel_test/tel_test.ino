int i = 0;
void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  String teld;
teld.concat("2082,");
teld.concat("15:23:21");
teld.concat(",");
teld.concat(String(i));
teld.concat(",");
teld.concat("F");
teld.concat(",");
teld.concat("DESCENT");
teld.concat(",");
teld.concat("765");
teld.concat(",");
teld.concat("15");
teld.concat(",");
teld.concat("N");
teld.concat(",");
teld.concat("C");
teld.concat(",");
teld.concat("32");
teld.concat(",");
teld.concat("7.2");
teld.concat(",");
teld.concat("97");
teld.concat(",");
//teld.concat(gpsV());
//teld.concat(",");
teld.concat("1.0,-0.6,0.4");

Serial.println(teld);

delay(1000);

i++;
}
