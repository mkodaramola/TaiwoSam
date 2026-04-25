/*
  BLE + MPU9250 Tilt Angle Notifier + Side Direction
  - Sends tilt angle + direction (Left/Centre/Right) via BLE Notify
  - Receives threshold value from BLE client
  - Vibrates when tilt < threshold
  - LED indicates BLE connection
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>

#include "MPU9250.h"
#include "eeprom_utils.h"

MPU9250 mpu;

// === BLE DEFINITIONS ===
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicNotify = NULL;
BLECharacteristic* pCharacteristicWrite = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs
#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "4fc09a67-d4a6-4857-98d0-a2b5d275ab9f"

// === DEVICE DEFINITIONS ===
byte vib = 4;
byte Bled = 3;

float tiltAngle = 0;
float threshold = 25.0;
 String pos = "Normal";


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

      Serial.print("Updated Threshold: ");
      Serial.println(threshold);
    }
  }
};


// === SETUP ===
void setup() {

  Serial.begin(115200);

  pinMode(vib, OUTPUT);
  pinMode(Bled, OUTPUT);
  digitalWrite(Bled, LOW);

  Wire.begin();
  delay(2000);

  // ---- MPU9250 Setup ----
  if (!mpu.setup(0x68)) {

    while (1) {
      Serial.println("MPU connection failed.");
      delay(5000);
    }
  }

  loadCalibration();
  mpu.verbose(false);


  // ---- BLE Setup ----
  BLEDevice::init("ESP32_Tilt_Sensor");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);


  // Notify characteristic
  pCharacteristicNotify = pService->createCharacteristic(
                            CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_NOTIFY
                          );

  pCharacteristicNotify->addDescriptor(new BLE2902());


  // Write characteristic
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

  Serial.println("Waiting for BLE client...");
}


// === LOOP ===
void loop() {

  if (mpu.update()) {

    static uint32_t prev_ms = millis();

    if (millis() > prev_ms + 500) {

      float yn = mpu.getYaw();
      float pn = mpu.getPitch();
      float rn = mpu.getRoll();

      float mx = mpu.getMagX();
      float my = mpu.getMagY();
      float mz = mpu.getMagZ();

      float ax = mpu.getAccX();
      float ay = mpu.getAccY();
      float az = mpu.getAccZ();


      float yc = yaw_compensated(yn, pn, rn, mx, my, mz);

      float rc = roll_compensated(ax, ay, az);


      tiltAngle = abs(pn);




      tiltAngle = map(tiltAngle, 0,79,0,45);


      Serial.print("Tilt: ");
      Serial.print(tiltAngle);
  
      Serial.print(" | Roll Comp: ");
      Serial.println(yc);

      Serial.print(" | Status: ");
      Serial.println(pos);

     

      if (yc <= 140)
        pos = "Abnornal(R)";

      else if (yc >= 180)
        pos = "Abnormal(L)";

      else
        pos = "Normal";



      // === BLE Notify ===
      if (deviceConnected) {

        String msg = String(tiltAngle,2) + "," + pos;

        pCharacteristicNotify->setValue(msg.c_str());
        pCharacteristicNotify->notify();

        Serial.println("BLE Notify sent: " + msg);
      }


      // === VIBRATION ===
      if (tiltAngle < threshold)
        analogWrite(vib, 64);

      else
        analogWrite(vib, 0);


      prev_ms = millis();
    }
  }


  // BLE reconnection handling
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



// === YAW COMPENSATION ===
float yaw_compensated(float yaw, float pitch, float roll,
                      float mx, float my, float mz)
{

  float pr = pitch * DEG_TO_RAD;
  float rr = roll  * DEG_TO_RAD;

  float Xh = mx * cos(pr) + mz * sin(pr);

  float Yh = mx * sin(rr) * sin(pr)
           + my * cos(rr)
           - mz * sin(rr) * cos(pr);

  float yaw_corrected = atan2(-Yh, Xh) * RAD_TO_DEG;

  if (yaw_corrected < 0)
      yaw_corrected += 360.0;

  return yaw_corrected;
}



// === ROLL COMPENSATION ===
float roll_compensated(float ax, float ay, float az)
{
  float roll_corrected = atan2(ay, az) * RAD_TO_DEG;

  return roll_corrected;
}