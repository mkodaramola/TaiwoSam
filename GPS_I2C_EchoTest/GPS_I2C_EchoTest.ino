// Test code for Adafruit GPS That Support Using I2C
//
// This code shows how to test a passthru between USB and I2C
//
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>

// Connect to the GPS on the hardware I2C port
Adafruit_GPS GPS(&Wire);


void setup() {
  // wait for hardware serial to appear
    while (!Serial);

    // make this baud rate fast enough to we aren't waiting on it
    Serial.begin(115200);

    Serial.println("Adafruit GPS library basic I2C test!");
    GPS.begin(0x10);  // The I2C address to use is 0x10
}
String h = "";
char c='0';
void loop() {



  if (GPS.available()) {
    c = GPS.read();
    h += c;
    //Serial.write(c);
    if(c == '$'){
       
      if(h.substring(0,5).equals("GNGGA") || h.substring(0,5).equals("GNRMC")) Serial.println(h);

      h = "";

      
  }

  }

  
}