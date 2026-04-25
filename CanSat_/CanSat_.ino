#include <Arduino_LPS22HB.h>
#include <Arduino_LSM9DS1.h>
//#include <SoftwareSerial.h>



int cal = 0;

float pressureToAltitude(float pressure) {

  // 101.325 - 93.100
  
  float seaLevelPressure = 101.325; // Standard pressure at sea level in hPa (1013.25 hPa or 101.325 kPa)
  float _altitude = 44330 * (1 - pow((pressure / seaLevelPressure), 0.1903));
  return _altitude;
}  


void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }


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
}

void loop() {
  
  float pressure = BARO.readPressure();
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" kPa");

  int Altitude = pressureToAltitude(pressure);
  Serial.print("Altitude = ");
  Serial.print(Altitude-cal);
  Serial.println(" m");

  float temperature = BARO.readTemperature();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" C");

   float Ax, Ay, Az;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(Ax, Ay, Az);
    Serial.print("Tilt X Y Z: ");
    Serial.print(Ax);
    Serial.print('\t');
    Serial.print(Ay);
    Serial.print('\t');
    Serial.println(Az);
  }

  float Gx, Gy, Gz;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(Gx, Gy, Gz);
    Serial.print("Gyro X Y Z: ");
    Serial.print(Gx);
    Serial.print('\t');
    Serial.print(Gy);
    Serial.print('\t');
    Serial.println(Gz);
  }

  if (Serial.available()) {
    Serial.write(Serial.read());
  }

  // print an empty line
  Serial.println();

  // wait 1 second to print again
  delay(1000);
}
