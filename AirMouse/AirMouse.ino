#include <Arduino_LSM9DS1.h>
#include <Mbed_BLE_HID.h>

MbedMouse mouse("Nano33_AirMouse");

float gx, gy, gz;
float sensitivity = 2.0;
float deadzone = 1.1;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("IMU init failed");
    while (1);
  }

  mouse.begin();
  Serial.println("BLE Air Mouse started");
}

void loop() {
  if (!mouse.isConnected()) {
    delay(10);
    return;
  }

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gx, gy, gz);

    int dx = processGyro(gy);
    int dy = processGyro(-gx);

    if (dx || dy) {
      mouse.move(dx, dy);
    }
  }

  delay(8);
}

int processGyro(float v) {
  if (abs(v) < deadzone) return 0;
  v *= sensitivity;
  v = constrain(v, -15, 15);
  return (int)v;
}
