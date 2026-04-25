#include "MPU9250.h"

#define MPU9250_IMU_ADDRESS 0x68
#define MAGNETIC_DECLINATION 1.63
#define INTERVAL_MS_PRINT 50

MPU9250 mpu;

unsigned long lastPrintMillis = 0;

/* ===== Orientation Reference =====
Set this when device is facing TRUE FRONT.
Get yaw value from serial monitor and update.
*/
float yaw_front_reference = 65.0;

/* ===== Normalize Angle ===== */
float normalizeAngle(float angle)
{
    while (angle > 180) angle -= 360;
    while (angle < -180) angle += 360;
    return angle;
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    Serial.println("Starting MPU9250...");

    MPU9250Setting setting;

    setting.accel_fs_sel = ACCEL_FS_SEL::A16G;
    setting.gyro_fs_sel = GYRO_FS_SEL::G1000DPS;
    setting.mag_output_bits = MAG_OUTPUT_BITS::M16BITS;
    setting.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_250HZ;

    setting.gyro_fchoice = 0x03;
    setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_20HZ;

    setting.accel_fchoice = 0x01;
    setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_45HZ;

    mpu.setup(MPU9250_IMU_ADDRESS, setting);

    mpu.setMagneticDeclination(MAGNETIC_DECLINATION);

    mpu.selectFilter(QuatFilterSel::MADGWICK);
    mpu.setFilterIterations(15);

    Serial.println("Calibration phase...");
    Serial.println("Place device on flat plane.");

    delay(5000);

    Serial.println("Ready!");
}

/* ===== Determine Orientation State ===== */
String getOrientationState(float sideAngle)
{
    if (sideAngle > -45 && sideAngle < 45)
        return "FRONT";

    if (sideAngle >= 45 && sideAngle < 135)
        return "RIGHT";

    if (sideAngle <= -45 && sideAngle > -135)
        return "LEFT";

    return "BACK";
}

void loop()
{
    unsigned long currentMillis = millis();

    if (mpu.update() && currentMillis - lastPrintMillis > INTERVAL_MS_PRINT)
    {
        float pitch = mpu.getPitch();
        float roll = mpu.getRoll();
        float yaw = mpu.getYaw();

        /* ===== Side Angle Computation ===== */
        float sideAngle = normalizeAngle(yaw - yaw_front_reference);

        String state = getOrientationState(sideAngle);

        Serial.println("------------");

        Serial.print("Pitch:\t");
        Serial.print(pitch);
        Serial.println("°");

        Serial.print("Roll:\t");
        Serial.print(roll);
        Serial.println("°");

        Serial.print("Yaw:\t");
        Serial.print(yaw);
        Serial.println("°");

        Serial.print("Side Angle:\t");
        Serial.print(sideAngle);
        Serial.println("°");

        Serial.print("Orientation:\t");
        Serial.println(state);

        Serial.println();

        lastPrintMillis = currentMillis;
    }
}