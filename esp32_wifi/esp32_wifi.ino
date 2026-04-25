#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

const char* ssid = "JsIndustries";
const char* password = "Jesussaves";
char ssidBuffer[20];
char passwordBuffer[20];
const int led = 5;

WiFiServer server(80);

void setup() {
  pinMode(led,OUTPUT);
  Serial.begin(9600);
  delay(10);

   strcpy(ssidBuffer, ssid);
  strcpy(passwordBuffer, password);


  // Connect to Wi-Fi network
  WiFi.begin(ssidBuffer, passwordBuffer);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  

  while (WiFi.status() != WL_CONNECTED){

    delay(500);
    Serial.print(".");
    }

   Serial.println("");
   Serial.println("WiFi connected");

    server.begin();
    Serial.println("Server Started");

    Serial.println(WiFi.localIP());

  
  

}

void loop() {

  WiFiClient client = server.available();
  if (client){

    Serial.println("New client connected");

    // Wait for client to send data
    while (!client.available()) delay(1);

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    if(request.indexOf("/LED=ON") != -1){
      
      digitalWrite(led,HIGH);
      }
     else if(request.indexOf("/LED=OFF") != -1){
      digitalWrite(led,LOW);
      }

      //Build the UI for the web client

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("");
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");

      client.print("LED is currently: ");
      if(digitalRead(led)) client.print("ON");
      else client.print("OFF");

      
      client.println("<br><br>");
      client.println("<a href=\"/LED=ON\"><button>Turn ON LED</button></a><br>");
      client.println("<a href=\"/LED=ON\"><button>Turn OFF LED</button></a>");
      client.println("</html>");

      delay(1);

      Serial.println("Client disconnected");
      Serial.println("");
    
   
    }
  

}
