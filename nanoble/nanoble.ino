#include <ArduinoBLE.h>

// Define the BLE service and characteristic
BLEService myService("fff0"); // Custom service UUID
BLECharacteristic myCharacteristic("fff1", BLERead | BLEWrite, 20); // Characteristic for data transfer

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial);

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  // Add the characteristic to the service
  myService.addCharacteristic(myCharacteristic);

  // Add the service to the BLE stack
  BLE.addService(myService);

  // Set the local name and advertised service
  BLE.setLocalName("Nano33BLESense");
  BLE.setAdvertisedService(myService);

  // Start advertising
  BLE.advertise();

  Serial.println("Advertising started. Waiting for connections...");
}

void loop() {
  // Poll for BLE events
  BLE.poll();

  // Check if a central device is connected
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    // While the central is connected
    while (central.connected()) {
      // Check if the characteristic has been written to
      if (myCharacteristic.written()) {
        // Read the value written to the characteristic
        byte data[20];
        int dataLength = myCharacteristic.readValue(data, 20);

        // Print the received data to the Serial Monitor
        Serial.print("Received: ");
        for (int i = 0; i < dataLength; i++) {
          Serial.print((char)data[i]); // Print each character
        }
        Serial.println();
      }
    }

    // When the central disconnects
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}