// Test code for Adafruit GPS That Support Using I2C
//
// This code shows how to parse data from the I2C GPS
//
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>

Adafruit_GPS GPS(&Wire);
#define GPSECHO false

uint32_t timer = millis();


void setup()
{
  while (!Serial);  // uncomment to have the sketch wait until Serial is ready

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit I2C GPS library basic test!");
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(0x10);  // The I2C address to use is 0x10
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);

  // Ask for firmware version
  GPS.println(PMTK_Q_RELEASE);
}

void loop() // run over and over again
{
  char c = GPS.read();
  if (GPSECHO)
    if (c) Serial.print(c);
  if (GPS.newNMEAreceived()) {
   
    //Serial.println(GPS.lastNMEA()); 
    if (!GPS.parse(GPS.lastNMEA())) 
      return; 
  }

  if (millis() - timer > 2000) {
   Serial.println(gpsV());
   timer = millis();
  }
}


String gpsV(){

 
  
   String gv = "";
    
    if (GPS.hour < 10) { gv += "0"; }
    gv+=String(GPS.hour); gv+=":";
    if (GPS.minute < 10) { gv += "0";  }
    gv+=String(GPS.minute); gv+=":";
    if (GPS.seconds < 10) { gv += "0";  }
    gv+=String(GPS.seconds); 
    gv+=", ";

    
    gv+= String(GPS.altitude);
    gv += ", "; 
    

    if (GPS.fix) {
      
      gv+=String(GPS.latitude); gv+= String(GPS.lat);
      gv+=", ";
      
      gv+= String(GPS.longitude); gv+=String(GPS.lon);
      gv+=", ";
      
      gv+= String((int)GPS.satellites);

//    gv += ", "; 
//    gv+=String(GPS.day); gv+="/";
//    gv+=String(GPS.month); gv+="/20";
//    gv+=String(GPS.year);
    }
  
  
  return gv;
  }
