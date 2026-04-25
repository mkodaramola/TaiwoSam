#include <ESP8266WiFi.h>
#define led 14
boolean started = false;
byte rssiThres = 75;
byte rt = 75;
const char* hotspotSSID = "wistick";
const char* hotspotPassword = "thereisnospoon";
byte count = 0;


void setup()
{
    Serial.begin(115200);

    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.setAutoConnect(false);
    delay(100);
    pinMode(led, OUTPUT);
    Serial.println("Setup done");
}

void loop()
{
    String MymacAddress = WiFi.macAddress();
    Serial.println(MymacAddress);
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    }
    else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {

            // Print SSID and RSSI for each network found
            String ssid = WiFi.SSID(i);
            int rssi = abs(WiFi.RSSI(i));

            String ssid1 = ssid.substring(0, 7);
            String ssid2 = ssid.substring(7);

            if (isNumericString(ssid2))
                rssiThres = ssid2.toInt();
            else
                rssiThres = rt;

            if (ssid1 == hotspotSSID) {
                if (!started) {
                    startAlert();
                    started = true;
                }

                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                Serial.print(" (");
                Serial.print(rssi);
                Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");

             
                 if (rssi > rssiThres) count++;

                 else count = 0;
                 
                 Serial.print("Count: "); Serial.println(count);
                    


                if (rssi > rssiThres && count > 1) {

                    
                    digitalWrite(led, HIGH);

                       if (!WiFi.isConnected()) {
                        Serial.println("Connecting to hotspot...");
                        WiFi.begin(hotspotSSID, hotspotPassword);
                    }

                    
                  
                }
                else {
                    digitalWrite(led, LOW);

                      if (WiFi.isConnected()) {
                        Serial.println("Disconnecting from hotspot...");
                        WiFi.disconnect();
                    }

                    
                }
            }

            delay(10);
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    delay(3000);
}

void startAlert()
{
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
    delay(500);
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
    delay(500);
}

bool isNumericString(const String& str)
{
    if (str.length() == 0) {
        return false;  // Empty string
    }

    for (size_t i = 0; i < str.length(); i++) {
        if (!isDigit(str.charAt(i))) {
            return false;  // Non-numeric character found
        }
    }

    return true;
}
