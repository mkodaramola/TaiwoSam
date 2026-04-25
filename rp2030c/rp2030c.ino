#include <Arduino_LSM6DSOX.h>

void setup() {
  // put your setup code here, to run once:
   Serial.begin(9600);
  while (!Serial);

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
  // put your main code here, to run repeatedly:


  float ax, ay, az;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    Serial.print("Values : ");
    Serial.print((2*atan(-1*ax)) * (180.0/PI));
    Serial.print('\t');
    Serial.print((2*atan(-1*ay))*(180.0/PI));
//    Serial.print('\t');
//    Serial.println(az);
  }



  float gx, gy, gz;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gx, gy, gz);
//    Serial.print("Gyroscope: ");
//    Serial.print(gx);
//    Serial.print('\t');
//    Serial.print(gy);
    Serial.print('\t');
    Serial.println(gz);
  }


if (IMU.temperatureAvailable())
  {
    int temperature_int = 0;
    float temperature_float = 0;
    IMU.readTemperature(temperature_int);
    IMU.readTemperatureFloat(temperature_float);

    Serial.print("Temperature: ");
    Serial.print(temperature_int);
    Serial.print(" (");
    Serial.print(temperature_float);
    Serial.print(")");
    Serial.println(" °C");
  }


  Serial.println("---------------------------------------------\n");

  delay(500);

}
