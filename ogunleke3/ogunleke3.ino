// ================================================================
// SMART IRRIGATION CONTROL SYSTEM - ESP32 + Firebase REST
// ================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ================================================================
// WIFI CONFIGURATION
// ================================================================

#define WIFI_SSID       "ogunleke"
#define WIFI_PASSWORD   "ogunleke1234"

// ================================================================
// FIREBASE CONFIGURATION
// ================================================================

#define DATABASE_URL \
"https://ogunleke-smart-irrigation-default-rtdb.firebaseio.com"

#define DATABASE_AUTH_QUERY ""

// ================================================================
// PIN DEFINITIONS
// ================================================================

#define SOIL_SENSOR_1  34
#define SOIL_SENSOR_2  35
#define SOIL_SENSOR_3  32
#define SOIL_SENSOR_4  33
#define RELAY_PIN      4

// ================================================================
// RELAY LOGIC
// ================================================================
// true  = LOW turns relay ON
// false = HIGH turns relay ON

#define RELAY_ACTIVE_LOW false

// ================================================================
// AUTOMATIC CONTROL THRESHOLDS
// ================================================================

#define DRY_THRESHOLD 30
#define WET_THRESHOLD 60

// Average moisture at which an automatically running pump stops.
int threshold = 80;

// ================================================================
// SENSOR CALIBRATION
// ================================================================

const int ADC_DRY[4] =
{
    2694,
    2686,
    2678,
    2676
};

const int ADC_WET[4] =
{
    1258,
    1244,
    1232,
    1227
};

// ================================================================
// TIMING
// ================================================================

const unsigned long SENSOR_INTERVAL = 5000;
const unsigned long COMMAND_INTERVAL = 2000;

// ================================================================
// GLOBAL VARIABLES
// ================================================================

WiFiClientSecure sslClient;

int soilMoisture[4] = {0, 0, 0, 0};

bool pumpState = false;

String systemMode = "AUTO";

unsigned long lastSensorRead = 0;
unsigned long lastCommandCheck = 0;

// ================================================================
// FUNCTION DECLARATIONS
// ================================================================

void connectWiFi();
void readSoilSensors();
int convertToMoisturePercentage(int rawValue, int sensorIndex);
void controlPump();
void setPump(bool state);
int countDrySensors();
int getAverageMoisture();
void uploadMoistureData();
void uploadPumpState();
void uploadMode();
void uploadThreshold();
void readModeFromFirebase();
void readThresholdFromFirebase();
void readManualPumpCommand();
String cleanFirebaseString(String value);
String buildUrl(const String &path);
bool firebasePutRaw(const String &path, const String &jsonBody);
bool firebasePutInt(const String &path, int value);
bool firebasePutBool(const String &path, bool value);
bool firebasePutString(const String &path, const String &value);
bool firebaseGetRaw(const String &path, String &responseOut);

// ================================================================
// BUILD FIREBASE URL
// ================================================================

String buildUrl(const String &path)
{
    String url = String(DATABASE_URL) + path + ".json";

    if (strlen(DATABASE_AUTH_QUERY) > 0)
    {
        url += DATABASE_AUTH_QUERY;
    }

    return url;
}

// ================================================================
// FIREBASE REST HELPERS
// ================================================================

bool firebasePutRaw(const String &path, const String &jsonBody)
{
    HTTPClient http;
    String url = buildUrl(path);

    if (!http.begin(sslClient, url))
    {
        Serial.println("HTTP begin() failed (PUT).");
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    int httpCode = http.PUT(jsonBody);
    bool success = (httpCode == 200);

    if (!success)
    {
        Serial.print("Firebase PUT failed. HTTP code: ");
        Serial.println(httpCode);
    }

    http.end();
    return success;
}

bool firebasePutInt(const String &path, int value)
{
    return firebasePutRaw(path, String(value));
}

bool firebasePutBool(const String &path, bool value)
{
    return firebasePutRaw(path, value ? "true" : "false");
}

bool firebasePutString(const String &path, const String &value)
{
    String jsonBody = "\"" + value + "\"";
    return firebasePutRaw(path, jsonBody);
}

bool firebaseGetRaw(const String &path, String &responseOut)
{
    HTTPClient http;
    String url = buildUrl(path);

    if (!http.begin(sslClient, url))
    {
        Serial.println("HTTP begin() failed (GET).");
        return false;
    }

    int httpCode = http.GET();
    bool success = (httpCode == 200);

    if (success)
    {
        responseOut = http.getString();
    }
    else
    {
        Serial.print("Firebase GET failed. HTTP code: ");
        Serial.println(httpCode);
    }

    http.end();
    return success;
}

// ================================================================
// CLEAN FIREBASE STRING
// ================================================================
// Removes surrounding quotation marks repeatedly.
// This handles values such as:
//     MANUAL
//     "MANUAL"
//     ""MANUAL""
//

String cleanFirebaseString(String value)
{
    value.trim();

    while (
        value.length() >= 2 &&
        value.startsWith("\"") &&
        value.endsWith("\"")
    )
    {
        value = value.substring(1, value.length() - 1);
        value.trim();
    }

    return value;
}

// ================================================================
// SET PUMP
// ================================================================

void setPump(bool state)
{
    pumpState = state;

    if (RELAY_ACTIVE_LOW)
    {
        digitalWrite(RELAY_PIN, state ? LOW : HIGH);
    }
    else
    {
        digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    }

    Serial.print("Pump: ");
    Serial.println(pumpState ? "ON" : "OFF");
}

// ================================================================
// ADC TO MOISTURE PERCENTAGE
// ================================================================

int convertToMoisturePercentage(int rawValue, int sensorIndex)
{
    int percentage = map(
        rawValue,
        ADC_DRY[sensorIndex],
        ADC_WET[sensorIndex],
        0,
        100
    );

    return constrain(percentage, 0, 100);
}

// ================================================================
// READ SOIL SENSORS
// ================================================================

void readSoilSensors()
{
    const int sensorPins[4] =
    {
        SOIL_SENSOR_1,
        SOIL_SENSOR_2,
        SOIL_SENSOR_3,
        SOIL_SENSOR_4
    };

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("READING SOIL MOISTURE");
    Serial.println("--------------------------------");

    for (int i = 0; i < 4; i++)
    {
        long total = 0;
        const int numberOfSamples = 10;

        for (int j = 0; j < numberOfSamples; j++)
        {
            total += analogRead(sensorPins[i]);
            delay(5);
        }

        int rawValue = total / numberOfSamples;

        soilMoisture[i] =
            convertToMoisturePercentage(rawValue, i);

        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" | Raw ADC: ");
        Serial.print(rawValue);
        Serial.print(" | Moisture: ");
        Serial.print(soilMoisture[i]);
        Serial.println("%");
    }

    Serial.println("--------------------------------");
}

// ================================================================
// COUNT DRY SENSORS
// ================================================================

int countDrySensors()
{
    int drySensorCount = 0;

    for (int i = 0; i < 4; i++)
    {
        if (soilMoisture[i] <= DRY_THRESHOLD)
        {
            drySensorCount++;
        }
    }

    return drySensorCount;
}

// ================================================================
// GET AVERAGE MOISTURE
// ================================================================

int getAverageMoisture()
{
    int sum = 0;

    for (int i = 0; i < 4; i++)
    {
        sum += soilMoisture[i];
    }

    return sum / 4;
}

// ================================================================
// AUTOMATIC PUMP CONTROL
// ================================================================

void controlPump()
{
    int drySensorCount = countDrySensors();

    if (!pumpState)
    {
        // Pump OFF -> ON when at least 3 sensors are <= 30%.
        if (drySensorCount >= 3)
        {
            Serial.println("AUTO MODE: 3 or more sensors <= 30%");
            Serial.println("AUTO MODE: Turning PUMP ON");

            setPump(true);
            uploadPumpState();
        }
    }
    else
    {
        // Pump ON -> OFF when average moisture reaches threshold.
        int averageMoisture = getAverageMoisture();

        if (averageMoisture >= threshold)
        {
            Serial.print("AUTO MODE: Average moisture (");
            Serial.print(averageMoisture);
            Serial.print("%) reached threshold (");
            Serial.print(threshold);
            Serial.println("%)");
            Serial.println("AUTO MODE: Turning PUMP OFF");

            setPump(false);
            uploadPumpState();
        }
    }
}

// ================================================================
// UPLOAD SOIL MOISTURE
// ================================================================

void uploadMoistureData()
{
    for (int i = 0; i < 4; i++)
    {
        String path =
            "/smartIrrigation/sensors/soil" + String(i + 1);

        if (firebasePutInt(path, soilMoisture[i]))
        {
            Serial.print("Firebase: Sensor ");
            Serial.print(i + 1);
            Serial.print(" = ");
            Serial.print(soilMoisture[i]);
            Serial.println("%");
        }
    }
}

// ================================================================
// UPLOAD PUMP STATE AS A REAL BOOLEAN
// ================================================================

void uploadPumpState()
{
    bool success = firebasePutBool(
        "/smartIrrigation/system/pump",
        pumpState
    );

    if (success)
    {
        Serial.print("Firebase pump state: ");
        Serial.println(pumpState ? "ON" : "OFF");
    }
}

// ================================================================
// UPLOAD MODE AS A STRING
// ================================================================

void uploadMode()
{
    firebasePutString(
        "/smartIrrigation/system/mode",
        systemMode
    );
}

// ================================================================
// UPLOAD THRESHOLD AS A REAL INTEGER
// ================================================================

void uploadThreshold()
{
    bool success = firebasePutInt(
        "/smartIrrigation/system/threshold",
        threshold
    );

    if (success)
    {
        Serial.print("Firebase threshold: ");
        Serial.print(threshold);
        Serial.println("%");
    }
}

// ================================================================
// READ MODE FROM FIREBASE
// ================================================================

void readModeFromFirebase()
{
    String response;

    if (!firebaseGetRaw(
            "/smartIrrigation/system/mode",
            response))
    {
        Serial.println("Mode read error.");
        return;
    }

    response.trim();

    if (response.length() == 0 || response == "null")
    {
        return;
    }

    String requestedMode;
    StaticJsonDocument<64> doc;

    DeserializationError err =
        deserializeJson(doc, response);

    if (!err && !doc.isNull())
    {
        requestedMode = doc.as<String>();
    }
    else
    {
        // Handles badly formatted values such as ""MANUAL"".
        requestedMode = response;
    }

    requestedMode = cleanFirebaseString(requestedMode);
    requestedMode.trim();
    requestedMode.toUpperCase();

    if (
        requestedMode != "AUTO" &&
        requestedMode != "MANUAL"
    )
    {
        Serial.print("Invalid mode received from Firebase: ");
        Serial.println(requestedMode);
        return;
    }

    if (requestedMode != systemMode)
    {
        Serial.print("Operating mode changed from ");
        Serial.print(systemMode);
        Serial.print(" to ");
        Serial.println(requestedMode);

        systemMode = requestedMode;

        if (systemMode == "AUTO")
        {
            controlPump();
        }
    }

}

// ================================================================
// READ THRESHOLD FROM FIREBASE
// ================================================================
// Accepts both:
//     75
// and:
//     "75"
// It also handles extra quotation marks.

void readThresholdFromFirebase()
{
    String response;

    if (!firebaseGetRaw(
            "/smartIrrigation/system/threshold",
            response))
    {
        Serial.println("Threshold read error.");
        return;
    }

    response.trim();

    if (response.length() == 0 || response == "null")
    {
        return;
    }

    StaticJsonDocument<64> doc;
    DeserializationError err =
        deserializeJson(doc, response);

    int requestedThreshold = threshold;
    bool validValue = false;

    if (!err && !doc.isNull())
    {
        if (doc.is<const char*>())
        {
            String value = doc.as<String>();
            value = cleanFirebaseString(value);
            value.trim();

            if (value.length() > 0)
            {
                requestedThreshold = value.toInt();
                validValue = true;
            }
        }
        else if (doc.is<int>() || doc.is<long>() || doc.is<unsigned int>())
        {
            requestedThreshold = doc.as<int>();
            validValue = true;
        }
    }
    else
    {
        // Handles badly formatted values such as ""75"".
        String value = cleanFirebaseString(response);
        value.trim();

        if (value.length() > 0)
        {
            requestedThreshold = value.toInt();
            validValue = true;
        }
    }

    if (!validValue)
    {
        Serial.println("Invalid threshold received from Firebase.");
        return;
    }

    requestedThreshold = constrain(requestedThreshold, 0, 100);

    if (requestedThreshold != threshold)
    {
        Serial.print("Firebase threshold changed from ");
        Serial.print(threshold);
        Serial.print("% to ");
        Serial.print(requestedThreshold);
        Serial.println("%");

        threshold = requestedThreshold;
    }

}

// ================================================================
// READ MANUAL PUMP COMMAND
// ================================================================
// Accepts both:
//     true / false          -> Boolean
// and:
//     "true" / "false"      -> String
// It also handles extra quotation marks.

void readManualPumpCommand()
{
    if (systemMode != "MANUAL")
    {
        return;
    }

    String response;

    if (!firebaseGetRaw(
            "/smartIrrigation/system/pump",
            response))
    {
        Serial.println("Manual pump command error.");
        return;
    }

    response.trim();

    if (response.length() == 0 || response == "null")
    {
        return;
    }

    StaticJsonDocument<64> doc;
    DeserializationError err =
        deserializeJson(doc, response);

    bool requestedPumpState = false;
    bool validValue = false;

    if (!err && !doc.isNull())
    {
        // Normal Boolean value: true or false
        if (doc.is<bool>())
        {
            requestedPumpState = doc.as<bool>();
            validValue = true;
        }
        // String value: "true" or "false"
        else if (doc.is<const char*>())
        {
            String value = doc.as<String>();
            value = cleanFirebaseString(value);
            value.trim();
            value.toLowerCase();

            if (
                value == "true" ||
                value == "1" ||
                value == "on"
            )
            {
                requestedPumpState = true;
                validValue = true;
            }
            else if (
                value == "false" ||
                value == "0" ||
                value == "off"
            )
            {
                requestedPumpState = false;
                validValue = true;
            }
        }
    }
    else
    {
        // Handles badly formatted values such as ""true"".
        String value = cleanFirebaseString(response);
        value.trim();
        value.toLowerCase();

        if (
            value == "true" ||
            value == "1" ||
            value == "on"
        )
        {
            requestedPumpState = true;
            validValue = true;
        }
        else if (
            value == "false" ||
            value == "0" ||
            value == "off"
        )
        {
            requestedPumpState = false;
            validValue = true;
        }
    }

    if (!validValue)
    {
        Serial.print("Invalid pump command received from Firebase: ");
        Serial.println(response);
        return;
    }

    if (requestedPumpState != pumpState)
    {
        Serial.print("MANUAL MODE: User requested pump ");
        Serial.println(requestedPumpState ? "ON" : "OFF");

        setPump(requestedPumpState);
    }

}

// ================================================================
// WIFI
// ================================================================

void connectWiFi()
{
    Serial.println();
    Serial.println("Connecting to Wi-Fi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected.");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
}

// ================================================================
// SETUP
// ================================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" SMART IRRIGATION CONTROL SYSTEM (REST)");
    Serial.println("========================================");

    // Relay initialization
    pinMode(RELAY_PIN, OUTPUT);

    // Always start with the pump OFF.
    setPump(false);

    // ADC configuration
    analogReadResolution(12);

    analogSetPinAttenuation(SOIL_SENSOR_1, ADC_11db);
    analogSetPinAttenuation(SOIL_SENSOR_2, ADC_11db);
    analogSetPinAttenuation(SOIL_SENSOR_3, ADC_11db);
    analogSetPinAttenuation(SOIL_SENSOR_4, ADC_11db);

    // Wi-Fi
    connectWiFi();

    // Firebase HTTPS
    sslClient.setInsecure();

    // Read the existing Firebase settings instead of overwriting them.
    readModeFromFirebase();
    readThresholdFromFirebase();

    if (systemMode == "MANUAL")
    {
        readManualPumpCommand();
    }

    Serial.println();
    Serial.println("System startup complete.");
}

// ================================================================
// MAIN LOOP
// ================================================================

void loop()
{
    // Reconnect Wi-Fi if necessary.
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Wi-Fi disconnected. Reconnecting...");
        connectWiFi();
    }

    // Read sensors every 5 seconds.
    if (millis() - lastSensorRead >= SENSOR_INTERVAL)
    {
        lastSensorRead = millis();

        readSoilSensors();
        uploadMoistureData();

        if (systemMode == "AUTO")
        {
            controlPump();
        }
    }

    // Check Firebase commands/settings every 2 seconds.
    if (millis() - lastCommandCheck >= COMMAND_INTERVAL)
    {
        lastCommandCheck = millis();

        readModeFromFirebase();
        readThresholdFromFirebase();

        if (systemMode == "MANUAL")
        {
            readManualPumpCommand();
        }
    }
}
