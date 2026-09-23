// ============================================================================
// SMART IRRIGATION SYSTEM
// IoT-Based Smart Irrigation System Using Edge Computing
// ESP32 + Firebase Realtime Database + Soil Moisture + DHT11 + LCD + Relay
// ============================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

#define WIFI_SSID       "Premauda_STARLINK2"
#define WIFI_PASSWORD   "Technologies"

// ============================================================================
// FIREBASE CONFIGURATION
// ============================================================================

#define DATABASE_URL \
"https://smart-irrigation-a4fb5-default-rtdb.firebaseio.com"

#define DATABASE_AUTH_QUERY ""

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Soil moisture sensor
#define SOIL_SENSOR_PIN     34

// DHT11
#define DHT_PIN             27
#define DHT_TYPE            DHT11

// Relay control
#define RELAY_PIN           4

// I2C
#define I2C_SDA             21
#define I2C_SCL             22

// LCD
#define LCD_ADDRESS         0x27
#define LCD_COLUMNS         16
#define LCD_ROWS            2

// ============================================================================
// RELAY LOGIC
// ============================================================================
//
// true  = LOW turns relay ON
// false = HIGH turns relay ON
//
// The schematic uses an NPN transistor (BC547) to drive the relay.
// GPIO HIGH -> transistor ON -> relay ON.
//
// Therefore:
// RELAY_ACTIVE_LOW = false
// ============================================================================

#define RELAY_ACTIVE_LOW false

// ============================================================================
// AUTOMATIC CONTROL THRESHOLDS
// ============================================================================

// Pump turns ON when soil moisture is at or below this value.
#define DRY_THRESHOLD 60

// Pump turns OFF when soil moisture reaches this value.
int threshold = 80;

// ============================================================================
// SOIL SENSOR CALIBRATION
// ============================================================================
//
// Based on the calibration used in the reference firmware.
//
// Dry condition:
// ADC = 2694
//
// Wet condition:
// ADC = 1258
//
// Since the soil sensor produces a lower ADC value as the soil becomes wetter,
// the conversion maps:
//
// 2694 -> 0%
// 1258 -> 100%
// ============================================================================

const int ADC_DRY = 2694;
const int ADC_WET = 1258;

// ============================================================================
// TIMING
// ============================================================================

const unsigned long SENSOR_INTERVAL = 5000;
const unsigned long COMMAND_INTERVAL = 2000;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

WiFiClientSecure sslClient;

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(
    LCD_ADDRESS,
    LCD_COLUMNS,
    LCD_ROWS
);

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

int soilMoisture = 0;
int soilRawValue = 0;

float temperature = 0.0;
float humidity = 0.0;

bool pumpState = false;

String systemMode = "AUTO";

unsigned long lastSensorRead = 0;
unsigned long lastCommandCheck = 0;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void connectWiFi();

void readSensors();

int convertToMoisturePercentage(int rawValue);

void controlPump();

void setPump(bool state);

void uploadSensorData();

void uploadPumpState();

void uploadMode();

void uploadThreshold();

void updateLCD();

void readModeFromFirebase();

void readThresholdFromFirebase();

void readManualPumpCommand();

String cleanFirebaseString(String value);

String buildUrl(const String &path);

bool firebasePutRaw(
    const String &path,
    const String &jsonBody
);

bool firebasePutInt(
    const String &path,
    int value
);

bool firebasePutFloat(
    const String &path,
    float value
);

bool firebasePutBool(
    const String &path,
    bool value
);

bool firebasePutString(
    const String &path,
    const String &value
);

bool firebaseGetRaw(
    const String &path,
    String &responseOut
);

// ============================================================================
// BUILD FIREBASE URL
// ============================================================================

String buildUrl(const String &path)
{
    String url = String(DATABASE_URL) + path + ".json";

    if (strlen(DATABASE_AUTH_QUERY) > 0)
    {
        url += DATABASE_AUTH_QUERY;
    }

    return url;
}

// ============================================================================
// FIREBASE REST PUT
// ============================================================================

bool firebasePutRaw(
    const String &path,
    const String &jsonBody
)
{
    HTTPClient http;

    String url = buildUrl(path);

    if (!http.begin(sslClient, url))
    {
        Serial.println("HTTP begin() failed (PUT).");
        return false;
    }

    http.addHeader(
        "Content-Type",
        "application/json"
    );

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

// ============================================================================
// FIREBASE PUT INTEGER
// ============================================================================

bool firebasePutInt(
    const String &path,
    int value
)
{
    return firebasePutRaw(
        path,
        String(value)
    );
}

// ============================================================================
// FIREBASE PUT FLOAT
// ============================================================================

bool firebasePutFloat(
    const String &path,
    float value
)
{
    if (isnan(value))
    {
        return false;
    }

    String jsonBody = String(value, 1);

    return firebasePutRaw(
        path,
        jsonBody
    );
}

// ============================================================================
// FIREBASE PUT BOOLEAN
// ============================================================================

bool firebasePutBool(
    const String &path,
    bool value
)
{
    return firebasePutRaw(
        path,
        value ? "true" : "false"
    );
}

// ============================================================================
// FIREBASE PUT STRING
// ============================================================================

bool firebasePutString(
    const String &path,
    const String &value
)
{
    String jsonBody = "\"" + value + "\"";

    return firebasePutRaw(
        path,
        jsonBody
    );
}

// ============================================================================
// FIREBASE GET
// ============================================================================

bool firebaseGetRaw(
    const String &path,
    String &responseOut
)
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

// ============================================================================
// CLEAN FIREBASE STRING
// ============================================================================

String cleanFirebaseString(String value)
{
    value.trim();

    while (
        value.length() >= 2 &&
        value.startsWith("\"") &&
        value.endsWith("\"")
    )
    {
        value = value.substring(
            1,
            value.length() - 1
        );

        value.trim();
    }

    return value;
}

// ============================================================================
// SET PUMP
// ============================================================================

void setPump(bool state)
{
    pumpState = state;

    if (RELAY_ACTIVE_LOW)
    {
        digitalWrite(
            RELAY_PIN,
            state ? LOW : HIGH
        );
    }
    else
    {
        digitalWrite(
            RELAY_PIN,
            state ? HIGH : LOW
        );
    }

    Serial.print("Pump: ");
    Serial.println(
        pumpState ? "ON" : "OFF"
    );

    updateLCD();
}

// ============================================================================
// ADC TO MOISTURE PERCENTAGE
// ============================================================================

int convertToMoisturePercentage(
    int rawValue
)
{
    int percentage = map(
        rawValue,
        ADC_DRY,
        ADC_WET,
        0,
        100
    );

    return constrain(
        percentage,
        0,
        100
    );
}

// ============================================================================
// READ SOIL MOISTURE AND DHT11
// ============================================================================

void readSensors()
{
    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("READING SENSORS");
    Serial.println("--------------------------------");

    // ------------------------------------------------------------------------
    // SOIL MOISTURE
    // ------------------------------------------------------------------------

    long total = 0;

    const int numberOfSamples = 10;

    for (int i = 0; i < numberOfSamples; i++)
    {
        total += analogRead(
            SOIL_SENSOR_PIN
        );

        delay(5);
    }

    soilRawValue =
        total / numberOfSamples;

    soilMoisture =
        convertToMoisturePercentage(
            soilRawValue
        );

    Serial.print("Soil Sensor");
    Serial.print(" | Raw ADC: ");
    Serial.print(soilRawValue);
    Serial.print(" | Moisture: ");
    Serial.print(soilMoisture);
    Serial.println("%");

    // ------------------------------------------------------------------------
    // DHT11
    // ------------------------------------------------------------------------

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (
        !isnan(newHumidity) &&
        !isnan(newTemperature)
    )
    {
        humidity = newHumidity;
        temperature = newTemperature;

        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
    }
    else
    {
        Serial.println(
            "DHT11 reading failed."
        );
    }

    Serial.println("--------------------------------");

    updateLCD();
}

// ============================================================================
// AUTOMATIC PUMP CONTROL
// ============================================================================
//
// With one soil sensor:
//
// Pump OFF -> ON when soil moisture <= 30%
//
// Pump ON -> OFF when soil moisture >= threshold
//
// This is the one-sensor equivalent of the four-sensor logic in the reference
// firmware.
// ============================================================================

void controlPump()
{
    if (!pumpState)
    {
        // ------------------------------------------------------------
        // PUMP OFF -> ON
        // ------------------------------------------------------------

        if (soilMoisture <= DRY_THRESHOLD)
        {
            Serial.print(
                "AUTO MODE: Soil moisture ("
            );

            Serial.print(soilMoisture);

            Serial.println(
                "%) is at or below dry threshold."
            );

            Serial.println(
                "AUTO MODE: Turning PUMP ON"
            );

            setPump(true);

            uploadPumpState();
        }
    }
    else
    {
        // ------------------------------------------------------------
        // PUMP ON -> OFF
        // ------------------------------------------------------------

        if (soilMoisture >= threshold)
        {
            Serial.print(
                "AUTO MODE: Soil moisture ("
            );

            Serial.print(soilMoisture);

            Serial.print(
                "%) reached threshold ("
            );

            Serial.print(threshold);

            Serial.println("%)");

            Serial.println(
                "AUTO MODE: Turning PUMP OFF"
            );

            setPump(false);

            uploadPumpState();
        }
    }
}

// ============================================================================
// UPLOAD SENSOR DATA
// ============================================================================

void uploadSensorData()
{
    // ------------------------------------------------------------------------
    // SOIL MOISTURE
    // ------------------------------------------------------------------------

    String soilPath =
        "/smartIrrigation/sensors/soilMoisture";

    if (
        firebasePutInt(
            soilPath,
            soilMoisture
        )
    )
    {
        Serial.print(
            "Firebase: Soil Moisture = "
        );

        Serial.print(soilMoisture);

        Serial.println("%");
    }

    // ------------------------------------------------------------------------
    // SOIL RAW ADC
    // ------------------------------------------------------------------------

    String rawPath =
        "/smartIrrigation/sensors/soilRaw";

    if (
        firebasePutInt(
            rawPath,
            soilRawValue
        )
    )
    {
        Serial.print(
            "Firebase: Soil Raw ADC = "
        );

        Serial.println(soilRawValue);
    }

    // ------------------------------------------------------------------------
    // TEMPERATURE
    // ------------------------------------------------------------------------

    if (!isnan(temperature))
    {
        String temperaturePath =
            "/smartIrrigation/sensors/temperature";

        if (
            firebasePutFloat(
                temperaturePath,
                temperature
            )
        )
        {
            Serial.print(
                "Firebase: Temperature = "
            );

            Serial.print(
                temperature
            );

            Serial.println(" C");
        }
    }

    // ------------------------------------------------------------------------
    // HUMIDITY
    // ------------------------------------------------------------------------

    if (!isnan(humidity))
    {
        String humidityPath =
            "/smartIrrigation/sensors/humidity";

        if (
            firebasePutFloat(
                humidityPath,
                humidity
            )
        )
        {
            Serial.print(
                "Firebase: Humidity = "
            );

            Serial.print(
                humidity
            );

            Serial.println(" %");
        }
    }
}

// ============================================================================
// UPLOAD PUMP STATE
// ============================================================================

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
}

// ============================================================================
// UPLOAD MODE
// ============================================================================

void uploadMode()
{
    firebasePutString(
        "/smartIrrigation/system/mode",
        systemMode
    );
}

// ============================================================================
// UPLOAD THRESHOLD
// ============================================================================

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
}

// ============================================================================
// READ MODE FROM FIREBASE
// ============================================================================

void readModeFromFirebase()
{
    String response;

    if (
        !firebaseGetRaw(
            "/smartIrrigation/system/mode",
            response
        )
    )
    {
        Serial.println(
            "Mode read error."
        );

        return;
    }

    response.trim();

    if (
        response.length() == 0 ||
        response == "null"
    )
    {
        return;
    }

    String requestedMode;

    StaticJsonDocument<64> doc;

    DeserializationError err =
        deserializeJson(
            doc,
            response
        );

    if (
        !err &&
        !doc.isNull()
    )
    {
        requestedMode =
            doc.as<String>();
    }
    else
    {
        requestedMode =
            response;
    }

    requestedMode =
        cleanFirebaseString(
            requestedMode
        );

    requestedMode.trim();
    requestedMode.toUpperCase();

    if (
        requestedMode != "AUTO" &&
        requestedMode != "MANUAL"
    )
    {
        Serial.print(
            "Invalid mode received from Firebase: "
        );

        Serial.println(
            requestedMode
        );

        return;
    }

    if (
        requestedMode != systemMode
    )
    {
        Serial.print(
            "Operating mode changed from "
        );

        Serial.print(systemMode);

        Serial.print(
            " to "
        );

        Serial.println(
            requestedMode
        );

        systemMode =
            requestedMode;

        // ------------------------------------------------------------
        // SAFETY BEHAVIOUR WHEN CHANGING MODES
        // ------------------------------------------------------------

        if (systemMode == "AUTO")
        {
            // Immediately allow automatic logic to decide.
            controlPump();
        }

        updateLCD();
    }
}

// ============================================================================
// READ THRESHOLD FROM FIREBASE
// ============================================================================

void readThresholdFromFirebase()
{
    String response;

    if (
        !firebaseGetRaw(
            "/smartIrrigation/system/threshold",
            response
        )
    )
    {
        Serial.println(
            "Threshold read error."
        );

        return;
    }

    response.trim();

    if (
        response.length() == 0 ||
        response == "null"
    )
    {
        return;
    }

    StaticJsonDocument<64> doc;

    DeserializationError err =
        deserializeJson(
            doc,
            response
        );

    int requestedThreshold =
        threshold;

    bool validValue = false;

    if (
        !err &&
        !doc.isNull()
    )
    {
        if (
            doc.is<const char*>()
        )
        {
            String value =
                doc.as<String>();

            value =
                cleanFirebaseString(
                    value
                );

            value.trim();

            if (value.length() > 0)
            {
                requestedThreshold =
                    value.toInt();

                validValue = true;
            }
        }
        else if (
            doc.is<int>() ||
            doc.is<long>() ||
            doc.is<unsigned int>()
        )
        {
            requestedThreshold =
                doc.as<int>();

            validValue = true;
        }
    }
    else
    {
        String value =
            cleanFirebaseString(
                response
            );

        value.trim();

        if (value.length() > 0)
        {
            requestedThreshold =
                value.toInt();

            validValue = true;
        }
    }

    if (!validValue)
    {
        Serial.println(
            "Invalid threshold received from Firebase."
        );

        return;
    }

    requestedThreshold =
        constrain(
            requestedThreshold,
            0,
            100
        );

    if (
        requestedThreshold != threshold
    )
    {
        Serial.print(
            "Firebase threshold changed from "
        );

        Serial.print(threshold);

        Serial.print("% to ");

        Serial.print(
            requestedThreshold
        );

        Serial.println("%");

        threshold =
            requestedThreshold;

        // Immediately check whether the new
        // threshold should switch the pump OFF.
        if (systemMode == "AUTO")
        {
            controlPump();
        }
    }
}

// ============================================================================
// READ MANUAL PUMP COMMAND
// ============================================================================
//
// Accepted values:
//
// true
// false
// "true"
// "false"
// 1
// 0
// "on"
// "off"
// ============================================================================

void readManualPumpCommand()
{
    if (systemMode != "MANUAL")
    {
        return;
    }

    String response;

    if (
        !firebaseGetRaw(
            "/smartIrrigation/system/pump",
            response
        )
    )
    {
        Serial.println(
            "Manual pump command error."
        );

        return;
    }

    response.trim();

    if (
        response.length() == 0 ||
        response == "null"
    )
    {
        return;
    }

    StaticJsonDocument<64> doc;

    DeserializationError err =
        deserializeJson(
            doc,
            response
        );

    bool requestedPumpState =
        false;

    bool validValue = false;

    if (
        !err &&
        !doc.isNull()
    )
    {
        // ------------------------------------------------------------
        // Boolean
        // ------------------------------------------------------------

        if (doc.is<bool>())
        {
            requestedPumpState =
                doc.as<bool>();

            validValue = true;
        }

        // ------------------------------------------------------------
        // String
        // ------------------------------------------------------------

        else if (
            doc.is<const char*>()
        )
        {
            String value =
                doc.as<String>();

            value =
                cleanFirebaseString(
                    value
                );

            value.trim();
            value.toLowerCase();

            if (
                value == "true" ||
                value == "1" ||
                value == "on"
            )
            {
                requestedPumpState =
                    true;

                validValue = true;
            }
            else if (
                value == "false" ||
                value == "0" ||
                value == "off"
            )
            {
                requestedPumpState =
                    false;

                validValue = true;
            }
        }
    }
    else
    {
        String value =
            cleanFirebaseString(
                response
            );

        value.trim();
        value.toLowerCase();

        if (
            value == "true" ||
            value == "1" ||
            value == "on"
        )
        {
            requestedPumpState =
                true;

            validValue = true;
        }
        else if (
            value == "false" ||
            value == "0" ||
            value == "off"
        )
        {
            requestedPumpState =
                false;

            validValue = true;
        }
    }

    if (!validValue)
    {
        Serial.print(
            "Invalid pump command received from Firebase: "
        );

        Serial.println(response);

        return;
    }

    if (
        requestedPumpState != pumpState
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

        uploadPumpState();
    }
}

// ============================================================================
// LCD UPDATE
// ============================================================================

void updateLCD()
{
    lcd.clear();

    // ------------------------------------------------------------------------
    // LINE 1
    // ------------------------------------------------------------------------

    lcd.setCursor(0, 0);

    lcd.print("Soil:");

    lcd.print(soilMoisture);

    lcd.print("% ");

    if (pumpState)
    {
        lcd.print("P:ON");
    }
    else
    {
        lcd.print("P:OFF");
    }

    // ------------------------------------------------------------------------
    // LINE 2
    // ------------------------------------------------------------------------

    lcd.setCursor(0, 1);

    if (!isnan(temperature))
    {
        lcd.print("T:");
        lcd.print(
            temperature,
            0
        );
        lcd.print("C ");
    }
    else
    {
        lcd.print("T:--C ");
    }

    if (!isnan(humidity))
    {
        lcd.print("H:");
        lcd.print(
            humidity,
            0
        );
        lcd.print("%");
    }
    else
    {
        lcd.print("H:--%");
    }
}

// ============================================================================
// WIFI
// ============================================================================

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

// ============================================================================
// SETUP
// ============================================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println(
        "=========================================="
    );

    Serial.println(
        " SMART IRRIGATION SYSTEM"
    );

    Serial.println(
        " ESP32 + FIREBASE + EDGE CONTROL"
    );

    Serial.println(
        "=========================================="
    );

    // =========================================================================
    // PIN INITIALIZATION
    // =========================================================================

    pinMode(
        RELAY_PIN,
        OUTPUT
    );

    // Always start with pump OFF.
    setPump(false);

    // =========================================================================
    // ADC CONFIGURATION
    // =========================================================================

    analogReadResolution(12);

    analogSetPinAttenuation(
        SOIL_SENSOR_PIN,
        ADC_11db
    );

    // =========================================================================
    // I2C INITIALIZATION
    // =========================================================================

    Wire.begin(
        I2C_SDA,
        I2C_SCL
    );

    // =========================================================================
    // LCD INITIALIZATION
    // =========================================================================

    lcd.init();

    lcd.backlight();

    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print(
        "Smart Irrigation"
    );

    lcd.setCursor(0, 1);

    lcd.print(
        "Starting..."
    );

    // =========================================================================
    // DHT INITIALIZATION
    // =========================================================================

    dht.begin();

    // =========================================================================
    // WIFI
    // =========================================================================

    connectWiFi();

    // =========================================================================
    // FIREBASE HTTPS
    // =========================================================================

    sslClient.setInsecure();

    // =========================================================================
    // INITIAL SENSOR READ
    // =========================================================================

    readSensors();

    // =========================================================================
    // READ EXISTING FIREBASE SETTINGS
    // =========================================================================

    readModeFromFirebase();

    readThresholdFromFirebase();

    if (systemMode == "MANUAL")
    {
        readManualPumpCommand();
    }
    else
    {
        controlPump();
    }

    // =========================================================================
    // INITIAL FIREBASE SENSOR UPLOAD
    // =========================================================================

    uploadSensorData();

    uploadPumpState();

    Serial.println();

    Serial.println(
        "System startup complete."
    );

    Serial.println(
        "=========================================="
    );

    updateLCD();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop()
{
    // =========================================================================
    // WIFI RECONNECTION
    // =========================================================================

    if (
        WiFi.status() != WL_CONNECTED
    )
    {
        Serial.println(
            "Wi-Fi disconnected. Reconnecting..."
        );

        connectWiFi();
    }

    // =========================================================================
    // SENSOR READING
    // =========================================================================

    if (
        millis() - lastSensorRead >=
        SENSOR_INTERVAL
    )
    {
        lastSensorRead =
            millis();

        readSensors();

        uploadSensorData();

        // ---------------------------------------------------------------------
        // EDGE COMPUTING
        // ---------------------------------------------------------------------
        //
        // Automatic pump decisions are made locally by the ESP32.
        // Firebase is therefore not required to make the actual irrigation
        // decision.
        // ---------------------------------------------------------------------

        if (systemMode == "AUTO")
        {
            controlPump();
        }
    }

    // =========================================================================
    // FIREBASE COMMAND CHECK
    // =========================================================================

    if (
        millis() - lastCommandCheck >=
        COMMAND_INTERVAL
    )
    {
        lastCommandCheck =
            millis();

        // ---------------------------------------------------------------------
        // Read operating mode
        // ---------------------------------------------------------------------

        readModeFromFirebase();

        // ---------------------------------------------------------------------
        // Read automatic irrigation threshold
        // ---------------------------------------------------------------------

        readThresholdFromFirebase();

        // ---------------------------------------------------------------------
        // Read manual pump command
        // ---------------------------------------------------------------------

        if (systemMode == "MANUAL")
        {
            readManualPumpCommand();
        }
    }
}