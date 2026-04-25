#include "MPU9250.h"

MPU9250 mpu;

// ---------- Smoothing ----------
const float alpha = 0.90;

float smoothYaw   = 0;
float smoothPitch = 0;
float smoothRoll  = 0;

void setup() {


    Serial.begin(115200);
    Wire.begin();
    delay(2000);

    // ---------- MPU Settings ----------
    MPU9250Setting setting;

    setting.accel_fs_sel = ACCEL_FS_SEL::A4G;
    setting.gyro_fs_sel  = GYRO_FS_SEL::G500DPS;

    setting.mag_output_bits = MAG_OUTPUT_BITS::M16BITS;

    // Supported sample rate
    setting.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_200HZ;

    // Digital Low Pass Filters (supported values)
    setting.gyro_dlpf_cfg  = GYRO_DLPF_CFG::DLPF_20HZ;
    setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_21HZ;

    // Enable filtering
    setting.gyro_fchoice  = 0x00;
    setting.accel_fchoice = 0x00;

    // ---------- Start MPU ----------
    if (!mpu.setup(0x68, setting)) {
        while (1) {
            Serial.println("MPU connection failed!");
            delay(2000);
        }
    }

    Serial.println("MPU9250 Ready!");

    // Initialize smoothing values
    mpu.update();


    smoothYaw   = mpu.getYaw();
    smoothPitch = mpu.getPitch();
    smoothRoll  = mpu.getRoll();
}

void loop() {

    if (mpu.update()) {

        static uint32_t lastPrint = 0;

        // Output at 20Hz
        if (millis() - lastPrint >= 50) {
            lastPrint = millis();

            float yaw   = mpu.getYaw();
            float pitch = mpu.getPitch();
            float roll  = mpu.getRoll();

            // Smooth output
            smoothYaw   = alpha * smoothYaw   + (1 - alpha) * yaw;
            smoothPitch = alpha * smoothPitch + (1 - alpha) * pitch;
            smoothRoll  = alpha * smoothRoll  + (1 - alpha) * roll;

            Serial.print("Yaw: ");
            Serial.print(smoothYaw, 2);

            Serial.print(" | Pitch: ");

            Serial.print(smoothPitch, 2);

            Serial.print(" | Roll: ");
            Serial.println(smoothRoll, 2);
        }
    }
}
