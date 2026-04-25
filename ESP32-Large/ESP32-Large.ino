#include <Wire.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "WiFi.h"
#include <HTTPClient.h>

#include <WiFiClientSecure.h>



// Define RX pin and TX pin for GPS Module and general Baud rate
// Pins used: RX - D17, TX - D16
byte GPSRX = 16, GPSTX = 17;

byte GPSIndicator = 13;

// Define gps object and hardware serial
TinyGPSPlus gps;
HardwareSerial hardwareSerial(1);

// Define the chip select pin for the SD card module
// Pins used: MISO - D19, MOSI - D23, CS - D5, SCK - D18
#define SD_CS 5
const char * filename = "/data";
const char * ext = ".txt";
int count = 0;

// Headers of the CSV file
const char * csvHeaders = "AcX,AcY,AcZ,GyX,GyY,GyZ,Latitude,Longitude,Time,Speed,Vibration Detected,Temp";

// Create task to read from backup from file and upload it
TaskHandle_t UploadBackupTask;

// Add details for WIFI and https internet
const char* ssid = "MTN_4G_325845"; //choose your wireless ssid
const char* password =  "CF9675A7"; //put your wireless password here.
byte WIFIIndicator = 14; // Wifi indicator LED pin
WiFiClientSecure *client;

//create an HTTPClient instance
HTTPClient https;
bool serverConnected;

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
    Serial.begin(115200);

  // Set the vibration sensor pin as an input pin and other digital pins
  pinMode(WIFIIndicator, OUTPUT);
  pinMode(GPSIndicator, OUTPUT);
  

  // Initialize hardware serial for UART comm. for GPS module
  hardwareSerial.begin(9600, SERIAL_8N1, GPSRX, GPSTX);

  // Connect to WIFI and prepare https client
  serverConnected = setupWifiAndInternet();

  // Configure SD card module
  configureSDCardMod();

  Serial.println("Anomaly Detection Device Setup complete");

  // create a task that will be executed in the uploadBackup function, with priority 1 and executed on core 0 (the second core)
  // xTaskCreatePinnedToCore(
  //                   uploadBackup,   /* Task function. */
  //                   "UploadBackup",     /* name of task. */
  //                   10000,       /* Stack size of task */
  //                   NULL,        /* parameter of the task */
  //                   1,           /* priority of the task */
  //                   &UploadBackupTask,      /* Task handle to keep track of created task */
  //                   0);          /* pin task to core 0 */                  
  // delay(500);
}

// void sensorReading(void * pvParameters) {
void loop() {
  // for (;;){
  String fileName;

  // This sketch displays information every time a new NMEA sentence is correctly encoded.
  while (hardwareSerial.available() > 0) {
    gps.encode(hardwareSerial.read());
  }
  
  String data;
  data = createCSVRowFromData(true);

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    data = createCSVRowFromData(false);
  }

  // Convert std::string to const char*
  const char* datachar = data.c_str();
  Serial.println("Sending data to esp32 mini...");
  Serial.write(datachar);

  Serial.println("");
  Serial.println("");

  delay(500);
}

void configureSDCardMod() {
  bool isMounted = SD.begin(SD_CS);
  if(!isMounted) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  String fileName = String(filename) + String(count) + String(ext);
  File file = SD.open(fileName);
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, fileName, csvHeaders);
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();
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
    digitalWrite(WIFIIndicator, LOW); // turn on WIFI indicator LED
  } else {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(WIFIIndicator, HIGH); // turn on WIFI indicator LED
  }

  client = new WiFiClientSecure;
  client->setCACert(rootCACertificate);

  return https.begin(*client, serverName);
}

// saveInfoToSD saves the sensor data to SD card
String createCSVRowFromData(bool gpsIsAvailable) {
  String dataMessage;

  if (gpsIsAvailable) {
    dataMessage = fetchGPSData(dataMessage);
  } else {
    dataMessage += "N/A,N/A,N/A,N/A"; // Latitude, Longitude, Time and Speed
  }

  Serial.println("GPS Sensor Data: " + dataMessage);
  return dataMessage;
}

String fetchGPSData(String dataMessage) {
  // Location
  if (gps.location.isValid()) {
    dataMessage += String(gps.location.lat(), 6) + ",";
    dataMessage += String(gps.location.lng(), 6) + ",";
    digitalWrite(GPSIndicator, HIGH);
  } else {
    dataMessage += "N/A,N/A,";
    digitalWrite(GPSIndicator, LOW);
  }

  // Date/Time
  if (gps.date.isValid() && gps.time.isValid()){
    // Date
    dataMessage += String(gps.date.year()) + "-";

    if (gps.date.month() < 10) dataMessage += "0";
    dataMessage += String(gps.date.month()) + "-";

    if (gps.date.day() < 10) dataMessage += "0";
    dataMessage += String(gps.date.day()) + " ";
    
    // Time
    if (gps.time.hour() < 10) dataMessage += "0";
    dataMessage += String(gps.time.hour()) + ":";

    if (gps.time.minute() < 10) dataMessage += "0";
    dataMessage += String(gps.time.minute()) + ":";

    if (gps.time.second() < 10) dataMessage += "0";
    dataMessage += String(gps.time.second()) + ",";
  
  } else {
    dataMessage += "N/A,";
  }

  if (gps.speed.isValid()) {
    dataMessage += String(gps.speed.mps()) + ",";
  } else {
    dataMessage += "N/A,";
  }

  return dataMessage;
}

void makePOSTRequest(String body) {
  String fileName = String(filename) + String(count) + String(ext);

  if (WiFi.status()== WL_CONNECTED){
    digitalWrite(WIFIIndicator, HIGH);
  } else {
    digitalWrite(WIFIIndicator, LOW);

    Serial.println("WIFI Unavailable: Saving data to backup SD card...");

    appendFile(SD, fileName, body);

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
        Serial.println("Saving data to backup SD card...");

        appendFile(SD, fileName, body);
      }
    } else {
      Serial.printf("[HTTPS] connection POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      Serial.println("Saving data to backup SD card...");

      appendFile(SD, fileName, body);
    }
  }

  // Free resources
  https.end();
}

// Write to the SD card
void writeFile(fs::FS &fs, String path, const char * message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.println(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card 
void appendFile(fs::FS &fs, String path, String message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.println(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// read from SD card
String readFile(fs::FS &fs, String path){
  String content = "";  // Initialize an empty string to store the file content

  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      return "";  // Return an empty string if the file can't be opened
  }

  while(file.available()){
      content += (char)file.read();  // Append each character to the content string
  }
  file.close();

  return content;  // Return the complete file content as a string
}

// delete file from SD card
void deleteFile(fs::FS &fs, String path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

// networkCall reads from the file and makes a POST request to the remote server every 5 minutes.
void uploadBackup( void * pvParameters ) {
  unsigned long lastTime = 0;
  unsigned long timerDelay = 300000; // 5 minutes

  Serial.print("UploadBackupTask running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
  // Upload any backup data every 5 minutes
    if ((millis() - lastTime) > timerDelay) {
      Serial.print("Uploading backup task to remote server: ");

      int currentCount = count;
      count++;
      delay(200); // delay to allow any current process being saved to the file to be completed
    
      String fileName = String(filename) + String(count) + String(ext);

      // Read from file and send the content to remote server
      String content = readFile(SD, fileName); 
      makePOSTRequest(content);

      lastTime = millis();
    }
  }
}
