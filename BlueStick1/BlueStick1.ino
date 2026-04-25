#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {}
};


void setup() {
  Serial.begin(115200);
  Serial.println("Scanning for BLE devices...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  BLEScanResults foundDevices = pBLEScan->start(2, false);
  Serial.print("Found ");
  Serial.print(foundDevices.getCount());
  Serial.println(" devices");

  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice advertisedDevice = foundDevices.getDevice(i);
    std::string deviceName = advertisedDevice.getName();
    int rssi = advertisedDevice.getRSSI();

    if (!deviceName.empty()) {
      Serial.print("Device Name: ");
      Serial.print(deviceName.c_str());
      Serial.print(", RSSI: ");
      Serial.println(rssi);
    }
  }
  pBLEScan->clearResults();
  delay(2000);
}
