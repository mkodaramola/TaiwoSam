/*
  BLE + MPU9250 Tilt Angle Notifier + Receiver
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "MPU9250.h"
#include <Wire.h>

// === MPU9250 DEFINITIONS (EXACT STYLE) ===
#define MPU9250_IMU_ADDRESS 0x68
#define MAGNETIC_DECLINATION 1.63
#define INTERVAL_MS_PRINT 1000

MPU9250 mpu;
unsigned long lastPrintMillis = 0;

// === BLE DEFINITIONS ===
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicNotify = NULL;
BLECharacteristic* pCharacteristicWrite = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs
#define SERVICE_UUID         "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "4fc09a67-d4a6-4857-98d0-a2b5d275ab9f"

// === DEVICE DEFINITIONS ===
byte vib = 4;
byte Bled = 3;

float tiltAngle = 0;
float sideAngle = 0;
float threshold = 25.0;

// === BLE SERVER CALLBACKS ===
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
    digitalWrite(Bled, HIGH);
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE device disconnected");
    digitalWrite(Bled, LOW);
  }
};

// === BLE WRITE CALLBACK ===
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Received via BLE: ");
      Serial.println(rxValue);
      threshold = rxValue.toFloat();
      Serial.print("🔧 Updated Threshold: ");
      Serial.println(threshold);
    }
  }
};

// === SETUP ===
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Starting BLE Tilt Notifier + Receiver...");

  pinMode(vib, OUTPUT);
  pinMode(Bled, OUTPUT);
  digitalWrite(Bled, LOW);

  // ---- BLE Setup ----
  BLEDevice::init("ESP32_Tilt_Sensor");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristicNotify = pService->createCharacteristic(
                            CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_NOTIFY
                          );
  pCharacteristicNotify->addDescriptor(new BLE2902());

  pCharacteristicWrite = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_2,
                            BLECharacteristic::PROPERTY_WRITE
                          );
  pCharacteristicWrite->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x00);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for client connection...");

  // ---- MPU9250 SETUP (EXACT FROM YOUR CODE) ----
  Wire.begin();
  Serial.println("Starting...");

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

  // Serial.println("Calibration will start in 5sec.");
  // Serial.println("Please leave the device still on the flat plane.");
  // delay(5000);
  // Serial.println("Calibrating...");
  // mpu.calibrateAccelGyro();

  // Serial.println("Magnetometer calibration will start in 5sec.");
  // Serial.println("Please Wave device in a figure eight until done.");
  // delay(5000);
  // Serial.println("Calibrating...");
  // mpu.calibrateMag();

  Serial.println("Ready!");
}

// === LOOP ===
void loop() {

  unsigned long currentMillis = millis();

  // ---- MPU9250 UPDATE (EXACT STYLE) ----
  if (mpu.update() && currentMillis - lastPrintMillis > INTERVAL_MS_PRINT) {

    float pitch = mpu.getPitch();
    float roll  = mpu.getRoll();
    float yaw   = mpu.getYaw();

    // Map Pitch to tiltAngle like original logic
    tiltAngle = abs(pitch);
    if (tiltAngle > 45) tiltAngle = 45;

    sideAngle = yaw;

    Serial.print("Pitch (Tilt): ");
    Serial.print(pitch);
    Serial.print(" | TiltAngle: ");
    Serial.print(tiltAngle);

    Serial.print(" | Yaw (Side): ");
    Serial.print(yaw);

    Serial.print(" | Threshold: ");
    Serial.println(threshold);

    lastPrintMillis = currentMillis;

    // ---- BLE Notify ----
    if (deviceConnected) {
      char buf[10];
      dtostrf(tiltAngle, 4, 2, buf);
      pCharacteristicNotify->setValue(buf);
      pCharacteristicNotify->notify();
      Serial.println("BLE Notify sent: " + String(buf));
    }

    // ---- Vibration ----
    if (tiltAngle < threshold)
      analogWrite(vib, 0);
    else
      analogWrite(vib, 0);
  }

  // ---- BLE connection management ----
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Restart advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
