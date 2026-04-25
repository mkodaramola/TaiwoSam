#include <ESP8266WiFi.h>

// Replace with your network credentials
const char* ssid = "JsIndustries";
const char* password = "Jesussaves";

// Create an instance of the server
WiFiServer server(80);

// LED pin
const int ledPin = 5;

void setup() {
  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
  Serial.println("----------------------------");

  // Handle the request
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
  } else if (request.indexOf("/LED=OFF") != -1) {
    digitalWrite(ledPin, LOW);
  }

  // Send the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<h1>ESP8266 LED Control</h1>");

  // Display the current state of the LED
  client.println("<p>LED is now: " + String(digitalRead(ledPin)) + "</p>");

  // Provide links to control the LED
  client.println("<p><a href=\"/LED=ON\">Turn On LED</a></p>");
  client.println("<p><a href=\"/LED=OFF\">Turn Off LED</a></p>");
  client.println("</html>");

  delay(1);
}
