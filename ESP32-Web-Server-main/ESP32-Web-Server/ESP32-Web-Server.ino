#include <WiFi.h>
#include <ESP32Servo.h>
#include "DHT.h"

#define DHTPIN 27
#define MQ3PIN 33 // Define MQ3 sensor pin
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

Servo s1;
Servo s2;

// Network credentials Here
const char* ssid     = "nano";
const char* password = "nanotest";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  
  s1.attach(14); // Attach servo to GPIO 14
  s2.attach(26);

  dht.begin();

  WiFi.softAP(ssid, password);
  
  // Print IP address and start web server
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

bool grip = false;

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  // Read DHT sensor values
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Compute heat index in Celsius
  float hic = dht.computeHeatIndex(t, h, false);
  
  // Read MQ3 sensor value
  int mq3Value = analogRead(MQ3PIN);
  String coL = "";
  if (mq3Value <= 100) coL = "Good";
  else if (mq3Value > 100 && mq3Value < 200) coL = "Fair";
  else coL = "HIGH";
  String gp = "Open";
  

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // Print a message out in the serial port
    String currentLine = "";                // Make a String to hold incoming data from the client

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {             // If there's bytes to read from the client,
        char c = client.read();             // Read a byte, then
        Serial.write(c);                    // Print it out in the serial monitor
        header += c;
        if (c == '\n') {                    // If the byte is a newline character
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Update grip status based on HTTP request
            if (header.indexOf("GET /open") >= 0) {
              grip = false;
              gp = "Opened";
            } else if (header.indexOf("GET /close") >= 0) {
              grip = true; 
              gp = "Closed";            
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"3\">"); // Auto-refresh every 5 seconds
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>");
            client.println("html { font-family: monospace; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: yellowgreen; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 32px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: gray;}");
            client.println("table {margin: 20px auto; border-collapse: collapse; width: 80%;}");
            client.println("th, td {border: 1px solid black; padding: 10px; text-align: center;}");
            client.println("th {background-color: #4CAF50; color: white;}");
            client.println("</style></head>");
            
            client.println("<body><h1>AeroCare</h1>");
            client.println("<p>Drone Health Delivery</p>");
            
            client.println("<p><a href=\"/open\"><button class=\"button\">OPEN</button></a></p>");
            client.println("<p><a href=\"/close\"><button class=\"button button2\">CLOSE</button></a></p>");
            
            client.println("<table>");
            client.println("<tr><th>Parameter</th><th>Value</th></tr>");
            client.println("<tr><td>Temperature (°C)</td><td>" + String(t) + "</td></tr>");
            client.println("<tr><td>Humidity (%)</td><td>" + String(h) + "</td></tr>");
            client.println("<tr><td>Heat Index (°C)</td><td>" + String(hic) + "</td></tr>");
            client.println("<tr><td>CO Level</td><td>" + String(coL) + "</td></tr>");
            client.println("<tr><td>Gripper State</td><td>" + String(gp) + "</td></tr>");
            client.println("</table>");
            
            client.println("</body></html>");


            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  if (grip) _close();
  else _open();
}

void _close() {
  s1.write(80);
  s2.write(120);
}

void _open() {
  s1.write(170);
  s2.write(10);
}
