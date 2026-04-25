#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "wistick"
#define APPSK "thereisnospoon"
#endif

int indicator = 14;

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

/* Just a little test message. Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void (*resetFunc)(void) = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

   WiFi.setAutoConnect(false);

  pinMode(indicator,OUTPUT);
  pinMode(2,OUTPUT);

  digitalWrite(13,LOW);
  
}

void loop() {
  server.handleClient();

  int numDevices = WiFi.softAPgetStationNum();
  Serial.print("Connected devices: ");
  Serial.println(numDevices);


   if(numDevices >= 1) { 
    digitalWrite(indicator,HIGH);
    digitalWrite(13,LOW);
    delay(7000);
    resetFunc();
   }
   else{
    digitalWrite(indicator,LOW);
    digitalWrite(2,HIGH); 
   }

   
   
  delay(5000);
}
