#include <Wire.h>
#include <MS5611.h>
#include <MPU6050.h>

MS5611 ms5611;
MPU6050 mpu;

void setup() {
  // put your setup code here, to run once:
   Serial.begin(9600);
  Wire.begin();

 // ---------------- MS5611   
  if (!ms5611.begin()) {
    Serial.println("Failed to initialize MS5611 sensor!");
    while (1);
  }
  
  ms5611.setOversampling(MS5611_ULTRA_HIGH_RES);


  // ------------------  MPU6050/GY521
    mpu.initialize();
  
  // Calibrate gyroscope and accelerometer
  mpu.calibrateGyro();
  mpu.calibrateAccel();
  
  // Set gyro sensitivity to +/-250 degrees/sec
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  
  // Set accelerometer sensitivity to +/-2g
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  
  // Initialize MPU6050
  mpu.setDLPFMode(MPU6050_DLPF_184);
  mpu.setRate(4); // Set sample rate to 200Hz


  // -------------- HMC5883L

  if (!hmc5883l.begin()) {
    Serial.println("Failed to initialize HMC5883L sensor!");
    while (1);
  }
  
  hmc5883l.setScale(1.3);  // Set the magnetic scale (options: 0.88, 1.3, 1.9, 2.5, 4.0, 4.7, 5.6, 8.1)
  hmc5883l.setMeasurementMode(Measurement_Continuous);
  
}

void loop() {
  // put your main code here, to run repeatedly:



}




void PressTemp(){
  
  float temperature, pressure;
  
  ms5611.readTemperatureAndPressure(temperature, pressure);
  
  float altitude = ms5611.getAltitude(pressure);

  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.print(" degC");

  Serial.print("   Pressure: ");
  Serial.print(pressure, 2);
  Serial.print(" mbar");

  Serial.print("   Altitude: ");
  Serial.print(altitude, 2);
  Serial.println(" meters");
  
  delay(1000);
  
  
  
  }



  void GyroAccel(){

     int16_t ax, ay, az;
  int16_t gx, gy, gz;
  
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  
  float gyroX = gx / 131.0;
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;
  
  Serial.print("Accelerometer (g): ");
  Serial.print("X = ");
  Serial.print(accelX);
  Serial.print("   Y = ");
  Serial.print(accelY);
  Serial.print("   Z = ");
  Serial.println(accelZ);
  
  Serial.print("Gyroscope (degrees/sec): ");
  Serial.print("X = ");
  Serial.print(gyroX);
  Serial.print("   Y = ");
  Serial.print(gyroY);
  Serial.print("   Z = ");
  Serial.println(gyroZ);
  
  delay(1000); // Wait for 1 second before reading again
    
    
    }


  void magnetoPointError(){

      int x, y, z;
  
  hmc5883l.getHeading(&x, &y, &z);
  
  Serial.print("Magnetometer X: ");
  Serial.print(x);
  
  Serial.print("   Y: ");
  Serial.print(y);
  
  Serial.print("   Z: ");
  Serial.println(z);


  float currentHeading = compass.getHeading();

  // Calculate the pointing error
  float pointingError = targetHeading - currentHeading;
  
  // Adjust the pointing error to the range of -180 to 180 degrees
  if (pointingError > 180.0) {
    pointingError -= 360.0;
  } else if (pointingError < -180.0) {
    pointingError += 360.0;
  }
  
  // Print the pointing error
  Serial.print("Pointing Error: ");
  Serial.print(pointingError, 2);
  Serial.println(" degrees");
  
  delay(500);
    
    
    }
