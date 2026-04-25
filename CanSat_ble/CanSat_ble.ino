#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_HTS221.h>
#include <TimerOne.h>
#include <SD.h>

RTC_DS3231 rtc;
File myFile;

TinyGPS gps;
SoftwareSerial ss(4, 3);


//// -- IMU Var
float Ax, Ay, Az;
float Gx, Gy, Gz;
float Mx, My, Mz;

//// -- GPS Var
unsigned long Age=0;
float latitude=0, longitude=0, Altitude=0;
unsigned long Time = 0;
unsigned short Satellites=0;


//// -- EEPROM Var





//// ISR
void timeISR();


/// -- Pitot Tube Var
//Routine for calculating the velocity from 
//a pitot tube and MPXV7002DP pressure differential sensor

float V_0 = 5.0; // supply voltage to the pressure sensor
float rho = 1.204; // density of air 

// parameters for averaging and offset
byte offset = 0;
byte offset_size = 10;
byte veloc_mean_size = 20;
byte zero_span = 2;

void setup() {
  Serial.begin(9600);
  while (!Serial);

 ss.begin(9600);

 // Initialize the SD card at the specified chipSelect pin
  if (!SD.begin(10)) {
    Serial.println("SD card initialization failed!");
    return;
  }

// Setup for BARO Pressure & Temperature
  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  
// Setup for HTS Temperature and Humidity
   if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }


// Setup for IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in g's");
  Serial.println("X\tY\tZ");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
  Serial.println("X\tY\tZ");
  Serial.print("Magnetic field sample rate = ");
  Serial.print(IMU.magneticFieldSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Magnetic Field in uT");
  Serial.println("X\tY\tZ");


  // Interrupt Timer Setup
  Timer1.initialize(1000000); // Set the timer interval
  Timer1.attachInterrupt(timerISR); // Attach the ISR (interrupt service routine)



  // RTC setup
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set RTC to the date & time this sketch was compiled
  }



  // Pitot Tube Setup
  for (int ii=0;ii<offset_size;ii++){
    offset += analogRead(A1)-(1023/2);
  }
  offset /= offset_size;

    
}

void loop() {




}



void timerISR() {
  
  Serial.println("Interrupt occurred!");
  
}


float airSpeed(){


  float adc_avg = 0; float veloc = 0.0;
  
// average a few ADC readings for stability
  for (int ii=0;ii<veloc_mean_size;ii++){
    adc_avg+= analogRead(A0)-offset;
  }
  adc_avg/=veloc_mean_size;
  
  // make sure if the ADC reads below 512, then we equate it to a negative velocity
  if (adc_avg>512-zero_span and adc_avg<512+zero_span){
  } else{
    if (adc_avg<512){
      veloc = -sqrt((-10000.0*((adc_avg/1023.0)-0.5))/rho);
    } else{
      veloc = sqrt((10000.0*((adc_avg/1023.0)-0.5))/rho);
    }
  }
    
  return veloc;
  }

String _RTC(){

  DateTime now = rtc.now(); // Get the current time from RTC


  return String(now.hour()) + "|" + String(now.minute()) + "|" + String(now.second()) + "|" + String(now.day()) + "|" + String(now.month())+ "|" + String(now.year());
  
  
  }

void SD_write(String t){

   myFile = SD.open("data.txt", FILE_WRITE); // Change "data.txt" to your desired file name
  
  if (myFile) {
    // If the file opens successfully, write data to it
    myFile.println(t); // Example data to write
    myFile.close(); // Close the file
    
  } else {
    Serial.println("Error opening file for writing!");
  }
  
  
  }


//void SD_writeEEPROM(String t){
//
//   myFile = SD.open("data.txt", FILE_WRITE); // Change "data.txt" to your desired file name
//  
//  if (myFile) {
//    // If the file opens successfully, write data to it
//    myFile.println(t); // Example data to write
//    myFile.close(); // Close the file
//    
//  } else {
//    Serial.println("Error opening file for writing!");
//  }
//  
//  
//  }


//                            <GPS Start>
String GPS(){

  
 if (ss.available() > 0) {
    // Read characters from the GPS module
    char c = ss.read();
    unsigned long Time = 0;
    // Feed the received characters to the TinyGPS library for parsing
    if (gps.encode(c)) {
      
      
      // Get GPS data
      gps.f_get_position(&latitude, &longitude, &Age);
      Altitude = gps.f_altitude();
      Satellites = gps.satellites();
       unsigned long Date = 0, fixAge = 0;
      gps.get_datetime(&Date,&Time,&fixAge);

      return printTime(Time) + "|" + String(Altitude) + "|" + String(latitude) + "|" + String(longitude) + "|" + String(Satellites);
      
    }
  } 
  
  else{
    
      return printTime(Time) + "|" + String(Altitude) + "|" + String(latitude) + "|" + String(longitude) + "|" + String(Satellites);

    }


  }
String printTime(unsigned long time) {
  // Extract hours, minutes, and seconds
  int hours = time / 1000000;
  int minutes = (time / 10000) % 100;
  int seconds = (time / 100) % 100;

  return String(hours) + ":" + String(minutes) + ":" + String(seconds);
}
//                             </GPS Ends>



//                             <BARO Start>
float BARO_Pressure(){
  
  // read the sensor value
  float pressure = BARO.readPressure();

  // print the sensor value
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" kPa");

  return pressure;
  
  }
float pressureToAltitude(float pressure) {

  // 101.325 - 93.100
  
  float seaLevelPressure = 101.325; // Standard pressure at sea level in hPa (1013.25 hPa or 101.325 kPa)
  float _altitude = 44330 * (1 - pow((pressure / seaLevelPressure), 0.1903));
  return _altitude;
}  

float BARO_Temperature(){
   
  float temperature = BARO.readTemperature();

  // print the sensor value
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" C");

  // print an empty line
  Serial.println();

  return temperature;
  
}
//                             </BARO Ends>



//                             <IMU Start>
void Accelerometer(){

  if (IMU.accelerationAvailable()) {
    
    IMU.readAcceleration(Ax, Ay, Az);

    Serial.print(Ax);
    Serial.print('\t');
    Serial.print(Ay);
    Serial.print('\t');
    Serial.println(Az);
  }

}
void Gyroscope(){
  
  if (IMU.gyroscopeAvailable()) {
    
    IMU.readGyroscope(Gx, Gy, Gz);   
    Serial.print(Gx);
    Serial.print('\t');
    Serial.print(Gy);
    Serial.print('\t');
    Serial.println(Gz);
  }
 
  }
void Magnetometer(){

  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(Mx, My, Mz);

    Serial.print(Gx);
    Serial.print('\t');
    Serial.print(Gy);
    Serial.print('\t');
    Serial.println(Gz);
  }

}
//                             </IMU Ends>


//                             <HTS Starts>
float HTS_temperature(){

   return HTS.readTemperature();

}
float HTS_humidity(){
  
   return HTS.readHumidity();
}
//                             </HTS Ends>
