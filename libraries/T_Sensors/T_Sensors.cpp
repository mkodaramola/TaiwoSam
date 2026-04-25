#include "T_Sensors.h"
#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_HMC5883_U.h>

Adafruit_HMC5883_Unified mag;
Adafruit_BME280 bme;
MPU6050 mpu;


payload::payload(){
	
}

void payload::begin(){
	
	 Wire.begin();
	
	if (!bme.begin(0x76)) {
    // Handle initialization error
  }
  
  if (!mag.begin()) {
    // Handle initialization error
  }
	
	
  mpu.initialize();
	
}

float payload::temp(){
		return bme.readTemperature();
}

float payload::altitude(){
		return bme.readAltitude(SEALEVELPRESSURE_HPA)	
}

float payload::voltage(){
		return 6.4
}

float payload::gyroR(){
	int16_t rawValue = mpu.getRotationX();
  return rawValue / 131.0f;  // Convert to degrees per second
}

float payload::gyroP(){
int16_t rawValue = mpu.getRotationY();
  return rawValue / 131.0f;
}

float payload::gyroY(){
	
	 int16_t rawValue = mpu.getRotationZ();
  return rawValue / 131.0f;

}
float payload::accR(){
	
	int16_t rawValue = mpu.getAccelerationX();
  return rawValue / 16384.0f;

}

float payload::accP(){
	int16_t rawValue = mpu.getAccelerationY();
  return rawValue / 16384.0f; 

}

float payload::accY(){
	
	 int16_t rawValue = mpu.getAccelerationZ();
  return rawValue / 16384.0f;  // Convert to g-force

}

float payload::magR() {
  sensors_event_t event;
  mag.getEvent(&event);
  
  float heading = atan2(event.magnetic.y, event.magnetic.x) * (180.0 / PI);
  if (heading < 0) {
    heading += 360.0;
  }
  
  return heading;
}

float payload::magP() {
  sensors_event_t event;
  mag.getEvent(&event);
  
  float pitch = atan2(event.magnetic.x, event.magnetic.z) * (180.0 / PI);
  return pitch;
}

float payload::magY() {
  sensors_event_t event;
  mag.getEvent(&event);
  
  float yaw = atan2(event.magnetic.z, event.magnetic.y) * (180.0 / PI);
  return yaw;
}

float payload:pError(float targetHeading, float currentHeading){
	float error = targetHeading - currentHeading;
  if (error > 180.0) {
    error -= 360.0;
  } else if (error < -180.0) {
    error += 360.0;
  }

  return error;
	
	
}





