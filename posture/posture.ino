/*
  BLE + MPU6050 Tilt Angle Notifier + Receiver
  - Sends tilt angle (0–45°) to connected BLE client (Notify)
  - Receives threshold value or other data from BLE client (Write)
  - Vibrates when tilt < threshold
  - LED indicates BLE connection
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// === BLE DEFINITIONS ===
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicNotify = NULL;
BLECharacteristic* pCharacteristicWrite = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs
#define SERVICE_UUID         "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"   // Notify characteristic
#define CHARACTERISTIC_UUID_2 "4fc09a67-d4a6-4857-98d0-a2b5d275ab9f"  // Write characteristic

// === MPU6050 + DEVICE DEFINITIONS ===
Adafruit_MPU6050 mpu;
byte vib = 4;
byte Bled = 3;  // Blue LED pin

float side = 0;
float sideAngle = 0;

float tilt = 0;
float tiltAngle = 0;
float threshold = 25.0; // Default threshold

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

// === BLE WRITE CALLBACK (for receiving data) ===
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String rxValue = pCharacteristic->getValue();  // FIXED: use Arduino String
    if (rxValue.length() > 0) {
      Serial.print("Received via BLE: ");
      Serial.println(rxValue);
      threshold = rxValue.toFloat();  // Convert threshold
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

  // Notify Characteristic (for sending tilt data)
  pCharacteristicNotify = pService->createCharacteristic(
                            CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_NOTIFY
                          );
  pCharacteristicNotify->addDescriptor(new BLE2902());

  // Write Characteristic (for receiving threshold/data)
  pCharacteristicWrite = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_2,
                            BLECharacteristic::PROPERTY_WRITE
                          );
  pCharacteristicWrite->setCallbacks(new MyCallbacks());

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x00);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for client connection...");

  // ---- MPU6050 Setup ----
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  Serial.println("MPU6050 initialized!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// === LOOP ===
void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  tilt = abs(a.acceleration.x);
  tiltAngle = tilt * (45.0 / 9.55);
  if (tiltAngle > 45) tiltAngle = 45;

  float y = (a.acceleration.y);
  float z = (a.acceleration.z);


  Serial.print("Tilt Raw: ");
  Serial.print(tilt);
  Serial.print(" | Mapped: ");
  Serial.print(tiltAngle);
  Serial.print(" | Threshold: ");
  Serial.println(threshold);

  Serial.print("y Raw: ");
  Serial.println(y);

  Serial.print("z Raw: ");
  Serial.println(z);



  if (deviceConnected) {
    char buf[10];
    dtostrf(tiltAngle, 4, 2, buf);
    pCharacteristicNotify->setValue(buf);
    pCharacteristicNotify->notify();
    Serial.println("BLE Notify sent: " + String(buf));
  }

  // Vibration feedback
  if (tiltAngle < threshold)
    analogWrite(vib, 64);
  else
    analogWrite(vib, 0);

  // BLE connection management
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Restart advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(500);
}
