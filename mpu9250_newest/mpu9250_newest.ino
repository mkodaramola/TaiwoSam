#include "MPU9250.h"
#include "eeprom_utils.h"

MPU9250 mpu;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    delay(2000);

    if (!mpu.setup(0x68)) {  // change to your own address
        while (1) {
            Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
            delay(5000);
        }
    }

    loadCalibration();

    print_calibration();
    mpu.verbose(false);
}

void loop() {
    if (mpu.update()) {
        static uint32_t prev_ms = millis();
        if (millis() > prev_ms + 25) {
            print_roll_pitch_yaw();
            prev_ms = millis();
        }
    }
}

void print_roll_pitch_yaw() {
    float yn = mpu.getYaw();
    float pn = mpu.getPitch();
    float rn = mpu.getRoll();

    float mx = mpu.getMagX();
    float my = mpu.getMagY();
    float mz = mpu.getMagZ();

   float yc = yaw_compensated(yn, pn, rn, mx, my, mz);


    Serial.print("Yaw, Pitch, Roll: ");
    Serial.print(yc, 2);
    Serial.print(", ");
    Serial.print(pn, 2);
    Serial.print(", ");
    Serial.println(rn, 2);
    String pos = "Centre";
    if (yc < 150) pos = "Left";
    else if (yc >= 150 && yc < 170) pos = "Centre";
    else pos = "Right";
    Serial.print("Direction: "); Serial.println(pos);

}


float yaw_compensated(float yaw, float pitch, float roll,
                      float mx, float my, float mz)
{
    // Convert degrees → radians
    float pr = pitch * DEG_TO_RAD;
    float rr = roll  * DEG_TO_RAD;

    // Tilt compensated magnetic vector projection
    float Xh = mx * cos(pr) + mz * sin(pr);

    float Yh = mx * sin(rr) * sin(pr)
             + my * cos(rr)
             - mz * sin(rr) * cos(pr);

    float yaw_corrected = atan2(-Yh, Xh) * RAD_TO_DEG;

    if (yaw_corrected < 0)
        yaw_corrected += 360.0;

    return yaw_corrected;
}

void print_calibration() {
    Serial.println("< calibration parameters >");
    Serial.println("accel bias [g]: ");
    Serial.print(mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
    Serial.print(", ");
    Serial.print(mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
    Serial.print(", ");
    Serial.print(mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
    Serial.println();
    Serial.println("gyro bias [deg/s]: ");
    Serial.print(mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
    Serial.print(", ");
    Serial.print(mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
    Serial.print(", ");
    Serial.print(mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
    Serial.println();
    Serial.println("mag bias [mG]: ");
    Serial.print(mpu.getMagBiasX());
    Serial.print(", ");
    Serial.print(mpu.getMagBiasY());
    Serial.print(", ");
    Serial.print(mpu.getMagBiasZ());
    Serial.println();
    Serial.println("mag scale []: ");
    Serial.print(mpu.getMagScaleX());
    Serial.print(", ");
    Serial.print(mpu.getMagScaleY());
    Serial.print(", ");
    Serial.print(mpu.getMagScaleZ());
    Serial.println();
}
