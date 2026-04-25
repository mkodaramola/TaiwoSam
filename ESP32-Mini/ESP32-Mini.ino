#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>

#include <WiFiClientSecure.h>

// Define mpu6050 object
// Pins used: SDA - D8, SCL - D9
Adafruit_MPU6050 mpu;
float AcX, AcY, AcZ, GyX, GyY, GyZ, Tmp;
const int MPUIndicator = 3;

// Define the pin number for Vibration Sensor
// Pins used: D0 - D25
const int VIBPin = 1;

// Add details for WIFI and https internet
const char* ssid = "MTN_4G_325845"; //choose your wireless ssid
const char* password =  "CF9675A7"; //put your wireless password here.
WiFiClientSecure *client;

//create an HTTPClient instance
HTTPClient https;
bool serverConnected;

// Define the serial monitor baud rate
static const uint32_t Baud = 115200;

// server URL where the data will be sent to
const char* serverName = "https://anomaly-detection-9fml.onrender.com/log";

const char* rootCACertificate= \
  "-----BEGIN CERTIFICATE-----\n" \
"MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n" \
"VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n" \
"A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n" \
"WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n" \
"IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n" \
"AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n" \
"QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n" \
"HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n" \
"BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n" \
"9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n" \
"p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n" \
"-----END CERTIFICATE-----\n";

void setup() {
  // Start the serial communication for debugging
  Serial.begin(Baud);

  pinMode(MPUIndicator, OUTPUT);

  // Connect to WIFI and prepare https client
  serverConnected = setupWifiAndInternet();

  // Configure MPU6050 sensor
  configureMPU6050();

  Serial.println("Anomaly Detection Device Setup complete");
}

void loop() {
  int VibLevel;

  // Read data from MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Collect accelerometer readings
  AcX = a.acceleration.x;
  AcY = a.acceleration.y;
  AcZ = a.acceleration.z;

  // Collect gyroscope readings
  GyX = g.gyro.x;
  GyY = g.gyro.y;
  GyZ = g.gyro.z;

  // Collect temperature data
  Tmp = temp.temperature;

  // Read vibration sensor value
  VibLevel = digitalRead(VIBPin);

  String messageFromMini;
  if (Serial.available()) // while message received 
  {
    messageFromMini = Serial.readStringUntil('\n'); // read message 
    Serial.println(messageFromMini);
  }

  String data = createCSVRowFromData(AcX, AcY, AcZ, GyX, GyY, GyZ, Tmp, VibLevel, messageFromMini);

  makePOSTRequest(data);

  Serial.println("");
  Serial.println("");
  delay(1000);
}

// Read data from each register that stores data from mpu6050
void configureMPU6050() {
  Wire.begin(8, 9); // SDA on GPIO 8, SCL on GPIO 9 for ESP32-C3
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    digitalWrite(MPUIndicator, LOW);
  } else {
    Serial.println("MPU6050 Found!");
    digitalWrite(MPUIndicator, HIGH);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// saveInfoToSD saves the sensor data to SD card
String createCSVRowFromData(float AcX, float AcY, float AcZ, float GyX, float GyY, float GyZ, float Tmp, int VibLevel, String messageFromMini) {
  String dataMessage = String(AcX) + "," + String(AcY) + "," + String(AcZ) + "," + String(GyX) + "," + String(GyY) + "," + String(GyZ) + ",";
  dataMessage += messageFromMini;

  if (VibLevel) {
    dataMessage += "1,";
  } else {
    dataMessage += "0,";
  }

  dataMessage += String(Tmp);

  Serial.println("Sensor Data: " + dataMessage);
  return dataMessage;
}

void makePOSTRequest(String body) {
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("WIFI Unavailable: Sending data to save to backup SD card...");

    sendToESP32Mega(body);

    return;
  }

  if (serverConnected) {  // HTTPS
    Serial.print("[HTTPS] POST...\n");

    // start connection and send HTTP header
    int httpCode = https.POST(body);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        // print server response payload
        String payload = https.getString();
        Serial.println(payload);
      } else {
        Serial.printf("[HTTPS] call POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        Serial.println("Sending data to save to backup SD card...");

        sendToESP32Mega(body);
      }
    } else {
      Serial.printf("[HTTPS] connection POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      Serial.println("Sending data to save to backup SD card...");

      sendToESP32Mega(body);
    }
  }

  // Free resources
  https.end();
}

bool setupWifiAndInternet() {
  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
    count++;

    if (count >= 5) {
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Could not connect to a WIFI network");
  } else {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  }

  client = new WiFiClientSecure;
  client->setCACert(rootCACertificate);

  return https.begin(*client, serverName);
}

void sendToESP32Mega(String data) {
  const char* datachar = data.c_str();
  Serial.write(datachar);
  delay(20);
}
