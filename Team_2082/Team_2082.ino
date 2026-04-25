#include <Wire.h>
#include <RtcDS3231.h>
#include <Adafruit_GPS.h>
#include <SD.h>
#include <SPI.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_HS300x.h>
#include "Arduino_BMI270_BMM150.h"


RtcDS3231<TwoWire> Rtc(Wire);
Adafruit_GPS GPS(&Wire); 
File myFile;
#define GPSECHO false
#define PITOT_ADDRESS 0x28
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9
// 1 = Clockwise
// 0 = Anti Clockwise


// Function Prototypes
String gpsV();
String imu_();
void M_write(String, String, boolean);
String printDateTime();
bool wasError();
float airspeed();
void Step(float,boolean, int);
String recovery_data(String, byte);
float pressureToAltitude(float);



float alt = 0.0;
int arr[2]={0,0};
unsigned long timer = 0;
String CSstate = "LAUNCH_WAIT";
unsigned int packet_count = 0;
String Temp="0";
boolean desc = false;
byte deploy_nich = 3;
unsigned int cal_val = 344;
boolean flightMode = true;
String con = "00:00:00,0,0;F;A";
String mode = "F";
boolean noseCone_detached = false;
byte nich = 5;
boolean upp = true;
String PC = "N";
String HS = "N";

float vel_temp = 0;
float init_alt = 0;

volatile bool serialFlag = false;
volatile bool serial1Flag = false;
String serialData = "";
String serial1Data = "";


// -- Setup --------------------------------------------------------------------------------------------------------------------------------------------------

void setup () {

    Serial1.begin(9600);
    Serial.begin(9600);


    while (!Serial);  
      
      // Baro Setup
      if (!BARO.begin()) {
          Serial.println("Failed to initialize pressure sensor!");
          while (1);
      }


     // HS300x Setup 
    if (!HS300x.begin()) {
      //Serial.println("Failed to initialize humidity temperature sensor!");
      //while (1);
    }


    // IMU Setup
     if (!IMU.begin()) {
    //Serial.println("Failed to initialize IMU!");
    // Add more debugging information
    //Serial.println("Possible causes:");
    //Serial.println("1. Incorrect wiring or loose connections.");
    //Serial.println("2. Incorrect I2C address.");
    //Serial.println("3. Sensor not receiving power.");
    
          //while (1);
      }
      
      //Serial.print("Accelerometer sample rate = ");
      //Serial.print(IMU.accelerationSampleRate());
      
      //Serial.print("Gyroscope sample rate = ");
      //Serial.print(IMU.gyroscopeSampleRate());


     // SD Setup
     //Serial.print("Initializing SD card...");
  if (!SD.begin(10)) {
    //Serial.println("initialization failed!");
    //while (1);
  }
  //Serial.println("initialization done.");

    // RTC Setup
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.IsDateTimeValid()) {
        if (!wasError()) {
            Rtc.SetDateTime(compiled);
        }
    }
    if (!Rtc.GetIsRunning()) {
        if (!wasError()) {
            Rtc.SetIsRunning(true);
        }
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError()) {
        if (now < compiled) {
            Rtc.SetDateTime(compiled);
        }
    }
    Rtc.Enable32kHzPin(false);
    wasError();
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
    wasError();

    

    // GPS Setup
    //Serial.println("Adafruit I2C GPS library basic test!");
    GPS.begin(0x10);  // The I2C address to use is 0x10
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    GPS.sendCommand(PGCMD_ANTENNA);
    delay(1000);
    GPS.println(PMTK_Q_RELEASE);


    // Recovery
    unsigned int check_packetCount =  recovery_data("rd.txt",2).toInt();
    if(check_packetCount > 0) {

            packet_count = check_packetCount;

            //cal_val = recovery_data("rd.txt",3).toInt();

            if (recovery_data("rd.txt",4).equals("F")) flightMode = true;
            else flightMode = false;


            if (recovery_data("rd.txt",5).equals("D")) desc = true;
            else desc = false;
            
            // ("12:04:54,152,333;F;D")
      
      }
      else {

        M_write("fl.csv","TEAM_ID,MISSION_TIME,PACKET_COUNT,MODE,STATE,ALTITUDE,AIR_SPEED,HS_DEPLOYED,PC_DEPLOYED,TEMPERATURE,VOLTAGE,PRESSURE,GPS_TIME,GPS_ALTITUDE,GPS_LATITUDE,GPS_LONGITUDE,GPS_SATS,TILT_X,TILT_Y,ROT_Z,CMD_ECHO", false);
          
          // Listen for command to either enter Flight Mode or Simulation Mode
          // Listen for command to calibrate Altitude to zero        
        
        }

    
        Wire.begin();

        if (flightMode) mode = "F";
        else mode = "S";

        
        pinMode(IN1, OUTPUT);
        pinMode(IN2, OUTPUT);
        pinMode(IN3, OUTPUT);
        pinMode(IN4, OUTPUT);
        pinMode(nich,OUTPUT);

        noseCone_detached = false;

}

float Altitude = 0;

float AirSpeed = 0;

// --- Loop -------------------------------------------------------------------------------------------------------------------------------

void loop () {
  

  float pressure = BARO.readPressure();

  float Btemperature = BARO.readTemperature();

  float Htemperature = HS300x.readTemperature();
  
  float humidity    = HS300x.readHumidity();

  if(Btemperature != 0 && Htemperature != 0) Htemperature = (Htemperature+Btemperature)/2;

 float Altitude = pressureToAltitude(pressure);

  float voltage = analogRead(A0);
  voltage = ((voltage/700.0)*5.0) - 0.322;
  voltage += 4.1;

 Altitude = Altitude-cal_val-3;

  if (Altitude > 170) desc = true;

  if(arr[1]-arr[0]> 7)  CSstate = "ASCENT";
  else if(arr[1]-arr[0]<=-7) CSstate = "DESCENT";
  else { 
        if(Altitude <= 8 && !desc) CSstate = "LAUNCH_WAIT";
        else if (Altitude < 3 && desc) CSstate = "LANDED";     
        else CSstate = "";
        
        }


    char c = GPS.read();
  if (GPSECHO)
    if (c) //Serial.print(c);
  if (GPS.newNMEAreceived()) {
   
    if (!GPS.parse(GPS.lastNMEA())) 
      return; 
  }


  if(Altitude > 170) {

      digitalWrite(deploy_nich, HIGH);
    
    }
    else digitalWrite(deploy_nich,LOW);


  if(CSstate.equals("DESCENT") && Altitude < 170 && !noseCone_detached){
          
        noseCone_detached = true;
        
        Step(25*360,1,15);

        digitalWrite(nich,HIGH);
        delay(5000);
        digitalWrite(nich,LOW);

        PC = "C";      
          
    }
  


String dateTimeString = printDateTime();




//#2082,15:23:21,4,F,DESCENT,765,15,N,C,32,7.2,97,1.0,-0.6,0.4


if (Altitude < 0) Altitude = 0;
String teld;
teld.concat("2082,");
teld.concat(dateTimeString);
teld.concat(",");
teld.concat(String(packet_count));
teld.concat(",");
teld.concat(mode);
teld.concat(",");
teld.concat(CSstate);
teld.concat(",");
teld.concat(String(Altitude));
teld.concat(",");
teld.concat(String(AirSpeed));
teld.concat(",");
teld.concat(HS);
teld.concat(",");
teld.concat(PC);
teld.concat(",");
teld.concat(String(Htemperature));
teld.concat(",");
teld.concat(String(voltage));
teld.concat(",");
teld.concat(String(pressure));
//teld.concat(",");
//teld.concat(gpsV());
teld.concat(",");
teld.concat(imu_());


     if(millis() - timer >= 1000){
        
        arr[0] = arr[1];
        arr[1] = int(Altitude);
        
        M_write("fl.csv",teld,true);  

        AirSpeed = airspeed(Altitude);

       Serial1.println(teld);
       Serial.println(teld); 



        
        
//         if (Altitude < 765 && upp){ 
//          Altitude+=70;
//         }
//         
//        else Altitude-=50;

        if (Altitude > 120) {
          upp = false;
          HS = "P";
        }
             
        if (desc) con = dateTimeString + "," + String(packet_count) + "," + String(cal_val) + ";" + mode + ";" + "D";
        else con = dateTimeString + "," + String(packet_count) + "," + String(cal_val) + ";" + mode + ";" + "A";
        M_write("rd.txt",con, false);

        
        packet_count++;
        
        timer = millis();

           

      }


}





// -- FUNCTIONS -----------------------------------------------------------------------------------------------------------------------------------


String imu_(){
  
           float Ax = 0;
         float Ay = 0;
         float Az = 0;
      
         String Simu = ""; 
      
        if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(Ax, Ay, Az);
          
        }
      
        float Gx=0;
        float Gy=0; 
        float Gz=0;
      
        if (IMU.gyroscopeAvailable()) {
          IMU.readGyroscope(Gx, Gy, Gz);
        }

        return String(Ax) + "," + String(Ay) + "," + String(Gx);
  
  }

float pressureToAltitude(float pressure) {

  // 101.325 - 93.100
  
  float seaLevelPressure = 101.325; // Standard pressure at sea level in hPa (1013.25 hPa or 101.325 kPa)
  float _altitude = 44330 * (1 - pow((pressure / seaLevelPressure), 0.1903));
  return _altitude;
}


float airspeed(float curr_alt){



  float diff = abs(init_alt - curr_alt);
    
     diff = (vel_temp + diff)/2.0;
     
     vel_temp = diff;

     init_alt = curr_alt;
    
     return diff;


  
//   Wire.beginTransmission(PITOT_ADDRESS);
//  Wire.write(0x00); // Command to request airspeed data (this may vary based on sensor specifics)
//  Wire.endTransmission();
//  
//  Wire.requestFrom(PITOT_ADDRESS, 2); // Request 2 bytes from the se  nsor
//  
//  if (Wire.available() == 2) {
//    uint16_t rawAirspeed = Wire.read() << 8 | Wire.read(); // Combine two bytes to form a 16-bit value
//    
//    // Convert raw airspeed to actual airspeed (this conversion factor may vary based on sensor specifics)
//    float air_speed = rawAirspeed * 0.1; // Example conversion factor
//    return air_speed;
//  } else {
//    return -1; // Return an error value if the data is not available
//  }
  

  }


void M_write(String fname, String content, boolean append){

    myFile = SD.open(fname, FILE_WRITE);

 

  // if the file opened okay, write to it:
  if (myFile) {

     if (!append){  

      myFile = SD.open(fname, O_WRITE | O_TRUNC);
         
    }
    
    //Serial.print("Writing to test.txt...");
    myFile.println(content);
    // close the file:
    myFile.close();
    //Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    //Serial.println("error opening test.txt");
  }

   
  }


String recovery_data(String fname, byte n){
  String text = "";
  myFile = SD.open(fname);
  if (myFile) {

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      text += (char)myFile.read();
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    //Serial.println("error opening test.txt");
  }

  byte fc = text.indexOf(",");
  byte lc = text.lastIndexOf(",");
  byte fsc = text.indexOf(";");
  byte lsc = text.lastIndexOf(";");
  

  if (n == 1) text = text.substring(0,fc);
  else if (n == 2) text = text.substring(fc+1,lc);
  else if (n == 3) text = text.substring(lc+1,fsc);
  else if (n == 4) text = text.substring(fsc+1,lsc);
  else if (n == 5) text = text.substring(lsc);
 
    
  return text; 
  
  }


String gpsV(){
  
   String gv = "";
    
    if (GPS.hour < 10) { gv += "0"; }
    gv+=String(GPS.hour); gv+=":";
    if (GPS.minute < 10) { gv += "0";  }
    gv+=String(GPS.minute); gv+=":";
    if (GPS.seconds < 10) { gv += "0";  }
    gv+=String(GPS.seconds); 
    gv+=",";

    
    gv+= String(GPS.altitude);
    gv += ", "; 
    

    if (GPS.fix) {
      
      gv+=String(GPS.latitude); gv+= String(GPS.lat);
      gv+=", ";
      
      gv+= String(GPS.longitude); gv+=String(GPS.lon);
      gv+=", ";
      
      gv+= String((int)GPS.satellites);

    }
  
  
  return gv;
  }




bool wasError() {
    uint8_t error = Rtc.LastError();
    return error != 0;
}

String printDateTime() {
     RtcDateTime now = Rtc.GetDateTime();
    if (!wasError()) {
        char datestring[20];
        snprintf_P(datestring, 
                sizeof(datestring),
                PSTR("%02u:%02u:%02u"),
                now.Hour(),
                now.Minute(),
                now.Second() );
        return datestring;
    }
    return "";
}


void Step(float angle,boolean dir, int d){


  int s = (angle/360) * (48);
  
  for(int in=0;in<s;in++){
    
    
    if (dir){
      switch(in%4){
        case 0:
          digitalWrite(IN1,1);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;
         case 1:
          digitalWrite(IN1,0);
          digitalWrite(IN2,1);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;  
          case 2:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,1);
          digitalWrite(IN4,0); 
          break;
          case 3:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,1);
          break;
          default: ;      
        }
    }
    else{
      switch(in%4){
        case 3:
            digitalWrite(IN1,1);
            digitalWrite(IN2,0);
            digitalWrite(IN3,0);
            digitalWrite(IN4,0);
          break;
         case 2:
          digitalWrite(IN1,0);
          digitalWrite(IN2,1);
          digitalWrite(IN3,0);
          digitalWrite(IN4,0);
          break;
          case 1:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,1);
          digitalWrite(IN4,0);
          break;
          case 0:
          digitalWrite(IN1,0);
          digitalWrite(IN2,0);
          digitalWrite(IN3,0);
          digitalWrite(IN4,1);
          break;
          default: ;
        }
      }
  
    
    delay(d);
    }
  
  
  }
