#ifndef T_Sensors_H
#define T_Sensors_H

#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_HMC5883_U.h>

class payload {
public:
  payload();
  void begin();
  float temp();
  float altitude();
  float voltage();
  float gyroR();
  float gyroP();
  float gyroY();
  float accR();
  float accP();
  float accY();
  float magR();
  float magP();
  float magY();
  float pError(float targetHeading, float currentHeading);

private:
  Adafruit_HMC5883_Unified mag;
  Adafruit_BME280 bme;
  MPU6050 mpu;
};

#endif

