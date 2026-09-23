// ================================================================
// LIBRARIES
// ================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>   // Install "ArduinoJson" (Benoit Blanchon) via Library Manager


// ================================================================
// WIFI CONFIGURATION
// ================================================================

#define WIFI_SSID       "Premauda_STARLINK2"
#define WIFI_PASSWORD   "Technologies"


// ================================================================
// FIREBASE CONFIGURATION (REST, unauthenticated)
// ================================================================

// Base Realtime Database URL, no trailing slash.
#define DATABASE_URL \
"https://ogunleke-smart-irrigation-default-rtdb.firebaseio.com"

// If you ever add a database secret / auth token, you can append
// it as "?auth=YOUR_TOKEN" in buildUrl(). Left blank = unauthenticated.
#define DATABASE_AUTH_QUERY ""


// ================================================================
// PIN DEFINITIONS
// ================================================================

// Soil moisture sensors
#define SOIL_SENSOR_1  34
#define SOIL_SENSOR_2  35
#define SOIL_SENSOR_3  32
#define SOIL_SENSOR_4  33

// Relay
#define RELAY_PIN      4


// ================================================================
// RELAY LOGIC
// ================================================================
//
// Most relay modules are ACTIVE LOW.
//
// ACTIVE LOW:
// LOW  = Relay ON
// HIGH = Relay OFF
//
// If your relay works the opposite way, change this to false.
//

#define RELAY_ACTIVE_LOW false


// ================================================================
// SOIL MOISTURE CONTROL THRESHOLDS
// ================================================================

#define DRY_THRESHOLD  30

// This is no longer used to turn the pump off directly, but is
// kept as a starting value for the "threshold" variable below.
#define WET_THRESHOLD  60


// ================================================================
// AVERAGE MOISTURE THRESHOLD (turns the pump OFF)
// ================================================================
//
// The pump is only turned OFF once the AVERAGE of all 4 sensor
// readings (sum of the 4 readings divided by 4) reaches this
// value. This is a variable (not a fixed #define) because it is
// also sent to Firebase under /smartIrrigation/system/threshold,
// so it can be tracked or changed from there later if needed.
//

int threshold = 80;   // starts at 80%


// ================================================================
// SENSOR CALIBRATION
// ================================================================
//
// These values MUST eventually be calibrated using your actual
// four sensors.
//
// ADC_DRY:
// ADC value when sensor is considered 0% moisture.
//
// ADC_WET:
// ADC value when sensor is considered 100% moisture.
//
// The values below are only starting values.
//
// IMPORTANT:
// Each sensor can have a slightly different ADC response.
// Therefore, each sensor has its own calibration values.
//

const int ADC_DRY[4] =
{
    2694,   // Sensor 1
    2686,   // Sensor 2
    2678,   // Sensor 3
    2676    // Sensor 4
};

const int ADC_WET[4] =
{
    1258,   // Sensor 1
    1244,   // Sensor 2
    1232,   // Sensor 3
    1227    // Sensor 4
};


// ================================================================
// TIMING
// ================================================================

// How often the soil sensors are read
const unsigned long SENSOR_INTERVAL = 5000;

// How often Firebase commands are checked
const unsigned long COMMAND_INTERVAL = 2000;


// ================================================================
// NETWORK OBJECTS
// ================================================================

WiFiClientSecure sslClient;


// ================================================================
// SYSTEM VARIABLES
// ================================================================

// Moisture percentages
//
// These are the values that are displayed and uploaded.
// Raw ADC values are NOT uploaded.

int soilMoisture[4] =
{
    0,
    0,
    0,
    0
};


// Current pump state
bool pumpState = false;


// Current operating mode
//
// AUTO
// MANUAL

String systemMode = "AUTO";


// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastCommandCheck = 0;


// ================================================================
// FUNCTION DECLARATIONS
// ================================================================

void connectWiFi();

void readSoilSensors();

int convertToMoisturePercentage(
    int rawValue,
    int sensorIndex
);

void controlPump();

void setPump(bool state);

int countDrySensors();

int getAverageMoisture();

void uploadMoistureData();

void uploadPumpState();

void uploadMode();

void uploadThreshold();

void readModeFromFirebase();

void readManualPumpCommand();

String buildUrl(const String &path);

bool firebasePutInt(const String &path, int value);

bool firebasePutBool(const String &path, bool value);

bool firebasePutString(const String &path, const String &value);

bool firebaseGetRaw(const String &path, String &responseOut);


// ================================================================
// BUILD A FIREBASE REST URL FOR A GIVEN PATH
// ================================================================
//
// Example:
//   buildUrl("/smartIrrigation/system/pump")
//   -> https://.../smartIrrigation/system/pump.json
//

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
// LOW-LEVEL REST HELPERS
// ================================================================
//
// Firebase REST semantics used here:
//   PUT  -> overwrite the value at a path
//   GET  -> read the value at a path
//
// All calls are unauthenticated HTTPS requests. sslClient.setInsecure()
// is used below so we don't have to manage Firebase's root CA cert -
// fine for prototyping, not for a hardened production deployment.
//

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
        Serial.print(httpCode);
        Serial.print(" | URL: ");
        Serial.println(url);

        String payload = http.getString();
        if (payload.length() > 0)
        {
            Serial.print("Response: ");
            Serial.println(payload);
        }
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
    // JSON strings must be quoted.
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
        Serial.print(httpCode);
        Serial.print(" | URL: ");
        Serial.println(url);
    }

    http.end();

    return success;
}


// ================================================================
// SET PUMP
// ================================================================

void setPump(bool state)
{
    pumpState = state;


    if (RELAY_ACTIVE_LOW)
    {
        if (state)
        {
            digitalWrite(RELAY_PIN, LOW);
        }
        else
        {
            digitalWrite(RELAY_PIN, HIGH);
        }
    }
    else
    {
        if (state)
        {
            digitalWrite(RELAY_PIN, HIGH);
        }
        else
        {
            digitalWrite(RELAY_PIN, LOW);
        }
    }


    Serial.print("Pump: ");

    if (pumpState)
    {
        Serial.println("ON");
    }
    else
    {
        Serial.println("OFF");
    }
}


// ================================================================
// CONVERT ADC TO MOISTURE PERCENTAGE
// ================================================================

int convertToMoisturePercentage(
    int rawValue,
    int sensorIndex
)
{
    int dryValue = ADC_DRY[sensorIndex];

    int wetValue = ADC_WET[sensorIndex];


    /*
      Convert:

      Dry value -> 0%

      Wet value -> 100%
    */

    int percentage = map(
        rawValue,
        dryValue,
        wetValue,
        0,
        100
    );


    // Keep result between 0 and 100
    percentage = constrain(
        percentage,
        0,
        100
    );


    return percentage;
}


// ================================================================
// READ ALL SOIL SENSORS
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
        /*
          Take multiple ADC readings.

          This helps reduce small fluctuations
          caused by ADC noise.
        */

        long total = 0;

        const int numberOfSamples = 10;


        for (int j = 0; j < numberOfSamples; j++)
        {
            total += analogRead(
                sensorPins[i]
            );

            delay(5);
        }


        /*
          Raw ADC value is kept locally.

          It is NOT stored in Firebase.
        */

        int rawValue =
            total / numberOfSamples;


        /*
          Convert raw ADC value to
          moisture percentage.
        */

        soilMoisture[i] =
            convertToMoisturePercentage(
                rawValue,
                i
            );


        // Display information in Serial Monitor

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
// COUNT NUMBER OF DRY SENSORS
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
// GET AVERAGE MOISTURE (sum of all 4 sensors / 4)
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
    /*
      This function is only used in AUTO mode.

      ------------------------------------------------------------
      PUMP OFF -> ON
      ------------------------------------------------------------

      If at least 3 sensors are <= 30%.

      ------------------------------------------------------------
      PUMP ON -> OFF
      ------------------------------------------------------------

      Only when the AVERAGE of all 4 sensors (sum / 4)
      reaches "threshold" (starts at 80%).
    */


    int drySensorCount =
        countDrySensors();


    // ============================================================
    // PUMP IS CURRENTLY OFF
    // ============================================================

    if (!pumpState)
    {
        /*
          At least 3 sensors have reached
          the dry threshold.
        */

        if (drySensorCount >= 3)
        {
            Serial.println();
            Serial.println(
                "AUTO MODE: 3 or more sensors <= 30%"
            );

            Serial.println(
                "AUTO MODE: Turning PUMP ON"
            );


            setPump(true);


            /*
              Save the new pump state to Firebase.
            */

            uploadPumpState();
        }
    }


    // ============================================================
    // PUMP IS CURRENTLY ON
    // ============================================================

    else
    {
        /*
          Pump remains ON until the average of all
          four sensors reaches "threshold".
        */

        int averageMoisture = getAverageMoisture();

        if (averageMoisture >= threshold)
        {
            Serial.println();

            Serial.print(
                "AUTO MODE: Average moisture ("
            );

            Serial.print(averageMoisture);

            Serial.print("%) reached threshold (");

            Serial.print(threshold);

            Serial.println("%)");

            Serial.println(
                "AUTO MODE: Turning PUMP OFF"
            );


            setPump(false);


            /*
              Save the new pump state to Firebase.
            */

            uploadPumpState();
        }
    }
}


// ================================================================
// UPLOAD MOISTURE PERCENTAGES
// ================================================================

void uploadMoistureData()
{
    /*
      IMPORTANT:

      Only moisture percentage is sent.

      No raw ADC value is stored in Firebase.
    */

    for (int i = 0; i < 4; i++)
    {
        String path =
            "/smartIrrigation/sensors/soil" +
            String(i + 1);


        bool success =
            firebasePutInt(
                path,
                soilMoisture[i]
            );


        if (success)
        {
            Serial.print(
                "Firebase: Sensor "
            );

            Serial.print(i + 1);

            Serial.print(" = ");

            Serial.print(
                soilMoisture[i]
            );

            Serial.println("%");
        }
        else
        {
            Serial.print(
                "Firebase error for Sensor "
            );

            Serial.println(i + 1);
        }
    }
}


// ================================================================
// UPLOAD PUMP STATE
// ================================================================

void uploadPumpState()
{
    bool success =
        firebasePutBool(
            "/smartIrrigation/system/pump",
            pumpState
        );


    if (success)
    {
        Serial.print(
            "Firebase pump state: "
        );

        Serial.println(
            pumpState ? "ON" : "OFF"
        );
    }
    else
    {
        Serial.println(
            "Firebase pump update error."
        );
    }
}


// ================================================================
// UPLOAD OPERATING MODE
// ================================================================

void uploadMode()
{
    firebasePutString(
        "/smartIrrigation/system/mode",
        systemMode
    );
}


// ================================================================
// UPLOAD THRESHOLD
// ================================================================
//
// Sends the current "threshold" value to Firebase under the
// same "system" object as mode and pump.
//

void uploadThreshold()
{
    bool success =
        firebasePutInt(
            "/smartIrrigation/system/threshold",
            threshold
        );

    if (success)
    {
        Serial.print(
            "Firebase threshold: "
        );

        Serial.print(threshold);

        Serial.println("%");
    }
    else
    {
        Serial.println(
            "Firebase threshold update error."
        );
    }
}


// ================================================================
// READ OPERATING MODE FROM FIREBASE
// ================================================================

void readModeFromFirebase()
{
    String response;

    bool success =
        firebaseGetRaw(
            "/smartIrrigation/system/mode",
            response
        );

    if (!success)
    {
        Serial.println(
            "Mode read error."
        );

        return;
    }


    /*
      Firebase returns JSON. For a string value this will be
      something like:  "AUTO"  (including the quote characters).

      Parse it with ArduinoJson so we handle quoting/whitespace
      and a possible null response correctly.
    */

    StaticJsonDocument<64> doc;

    DeserializationError err = deserializeJson(doc, response);

    if (err)
    {
        Serial.print("Mode JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }

    if (doc.isNull())
    {
        // Path does not exist yet in the database.
        return;
    }

    String requestedMode = doc.as<String>();

    requestedMode.trim();


    /*
      Accept only:

      AUTO
      MANUAL
    */

    if (
        requestedMode == "AUTO" ||
        requestedMode == "MANUAL"
    )
    {
        if (requestedMode != systemMode)
        {
            Serial.println();

            Serial.print(
                "Operating mode changed from "
            );

            Serial.print(systemMode);

            Serial.print(" to ");

            Serial.println(requestedMode);


            systemMode =
                requestedMode;


            /*
              When changing to AUTO mode,
              immediately allow the automatic
              control logic to determine the
              correct pump state.
            */

            if (systemMode == "AUTO")
            {
                controlPump();
            }
        }
    }
}


// ================================================================
// READ MANUAL PUMP COMMAND
// ================================================================

void readManualPumpCommand()
{
    /*
      Manual pump control is completely
      ignored when the system is in AUTO mode.
    */

    if (systemMode != "MANUAL")
    {
        return;
    }


    String response;

    bool success =
        firebaseGetRaw(
            "/smartIrrigation/system/pump",
            response
        );

    if (!success)
    {
        Serial.println(
            "Manual pump command error."
        );

        return;
    }

    response.trim();

    if (response.length() == 0 || response == "null")
    {
        // Path does not exist yet.
        return;
    }

    bool requestedPumpState = (response == "true");


    /*
      Only change the physical relay
      if the requested state differs
      from the current state.
    */

    if (
        requestedPumpState !=
        pumpState
    )
    {
        Serial.print(
            "MANUAL MODE: User requested pump "
        );

        Serial.println(
            requestedPumpState
                ? "ON"
                : "OFF"
        );


        setPump(
            requestedPumpState
        );
    }
}


// ================================================================
// CONNECT TO WIFI
// ================================================================

void connectWiFi()
{
    Serial.println();
    Serial.println(
        "Connecting to Wi-Fi..."
    );


    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    while (
        WiFi.status() != WL_CONNECTED
    )
    {
        delay(500);

        Serial.print(".");
    }


    Serial.println();

    Serial.println(
        "Wi-Fi connected."
    );


    Serial.print(
        "ESP32 IP address: "
    );

    Serial.println(
        WiFi.localIP()
    );
}


// ================================================================
// SETUP
// ================================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);


    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        " SMART IRRIGATION CONTROL SYSTEM (REST)"
    );

    Serial.println(
        "========================================"
    );


    // ============================================================
    // RELAY INITIALIZATION
    // ============================================================

    pinMode(
        RELAY_PIN,
        OUTPUT
    );


    /*
      IMPORTANT SAFETY FEATURE:

      Always start with the pump OFF.
    */

    setPump(false);


    // ============================================================
    // ADC CONFIGURATION
    // ============================================================

    analogReadResolution(12);


    /*
      Use 11 dB attenuation for the four
      soil moisture ADC inputs.
    */

    analogSetPinAttenuation(
        SOIL_SENSOR_1,
        ADC_11db
    );

    analogSetPinAttenuation(
        SOIL_SENSOR_2,
        ADC_11db
    );

    analogSetPinAttenuation(
        SOIL_SENSOR_3,
        ADC_11db
    );

    analogSetPinAttenuation(
        SOIL_SENSOR_4,
        ADC_11db
    );


    // ============================================================
    // WIFI
    // ============================================================

    connectWiFi();


    // ============================================================
    // FIREBASE (REST, unauthenticated)
    // ============================================================

    /*
      Skip SSL certificate verification.

      This is convenient for development.
      For a production deployment, certificate
      verification should be configured properly
      (e.g. sslClient.setCACert(...) with Firebase's
      root CA).
    */

    sslClient.setInsecure();


    /*
      Push the initial mode, pump state, and threshold
      to Firebase so the database reflects what the
      device booted into.
    */

    uploadMode();

    uploadPumpState();

    uploadThreshold();


    Serial.println();

    Serial.println(
        "System startup complete."
    );
}


// ================================================================
// MAIN LOOP
// ================================================================

void loop()
{
    // ============================================================
    // MAKE SURE WIFI IS STILL CONNECTED
    // ============================================================

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "Wi-Fi disconnected. Reconnecting..."
        );

        connectWiFi();
    }


    // ============================================================
    // READ SOIL MOISTURE
    // ============================================================

    if (
        millis() - lastSensorRead >=
        SENSOR_INTERVAL
    )
    {
        lastSensorRead =
            millis();


        /*
          Read the four sensors.
        */

        readSoilSensors();


        /*
          Send only the moisture
          percentages to Firebase.
        */

        uploadMoistureData();


        /*
          AUTO mode:
          evaluate the irrigation condition.
        */

        if (systemMode == "AUTO")
        {
            controlPump();
        }
    }


    // ============================================================
    // CHECK MOBILE APP COMMANDS
    // ============================================================

    if (
        millis() - lastCommandCheck >=
        COMMAND_INTERVAL
    )
    {
        lastCommandCheck =
            millis();


        /*
          First check whether the
          user changed AUTO/MANUAL mode.
        */

        readModeFromFirebase();


        /*
          If MANUAL mode, check the
          pump command from the app.
        */

        if (systemMode == "MANUAL")
        {
            readManualPumpCommand();
        }
    }
}