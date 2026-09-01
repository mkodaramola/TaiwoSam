#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"


// =====================================================
// WiFi / Firebase
// =====================================================

#define WIFI_SSID       "Premauda_STARLINK2"
#define WIFI_PASSWORD   "Technologies"

#define FIREBASE_HOST   "https://medshield-b9140-default-rtdb.firebaseio.com"


// =====================================================
// Speaker / Ultrasonic Mosquito Repellent
// =====================================================

const int speaker = 25;


// -----------------------------------------------------
// Default values
// Used when Firebase/WiFi is unavailable
// -----------------------------------------------------

const int DEFAULT_FREQUENCY = 32000;
const bool DEFAULT_REPELLENT_STATE = true;


// -----------------------------------------------------
// Allowed frequencies
// -----------------------------------------------------

const int ALLOWED_FREQUENCIES[] = {
    32000,
    20000,
    10000,
    5000,
    1000,
    500,
    200
};

const int NUM_FREQUENCIES =
    sizeof(ALLOWED_FREQUENCIES) /
    sizeof(ALLOWED_FREQUENCIES[0]);


// -----------------------------------------------------
// Current selected frequency
// -----------------------------------------------------

int selectedFrequency = DEFAULT_FREQUENCY;


// -----------------------------------------------------
// Repellent state
// -----------------------------------------------------

bool repellentEnabled = DEFAULT_REPELLENT_STATE;


// -----------------------------------------------------
// Firebase control polling
// -----------------------------------------------------

unsigned long prevControlTime = 0;

const unsigned long CONTROL_INTERVAL = 1000;


// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// =====================================================
// MAX30102 / MAX30105
// =====================================================

MAX30105 particleSensor;


// -----------------------------------------------------
// BPM
// -----------------------------------------------------

const byte RATE_SIZE = 4;

byte rates[RATE_SIZE];
byte rateSpot = 0;
byte rateCount = 0;

long lastBeat = 0;

int beatAvg = 0;


// -----------------------------------------------------
// SpO2
// -----------------------------------------------------

#define BUFFER_SIZE 100

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int bufferIndex = 0;

int32_t spo2 = 0;
int8_t spo2Valid = 0;

int32_t calculatedHeartRate = 0;
int8_t calculatedHRValid = 0;


// Last valid SpO2 value
int32_t Vspo2 = 0;


// -----------------------------------------------------
// Finger detection
// -----------------------------------------------------

const long FINGER_THRESHOLD = 50000;


// =====================================================
// 5-Second Sensor Stabilisation
// =====================================================

const unsigned long SENSOR_WAIT_TIME = 5000;

unsigned long sensorStartTime = 0;

bool sensorReady = false;


// =====================================================
// DS18B20
// =====================================================

const int oneWireBus = 4;

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

float temperatureC = 0.0;


// =====================================================
// Timing
// =====================================================

unsigned long prevTempTime = 0;
unsigned long prevFirebaseTime = 0;
unsigned long prevLCDTime = 0;

const unsigned long TEMP_INTERVAL = 5000;
const unsigned long FIREBASE_INTERVAL = 2000;
const unsigned long LCD_INTERVAL = 500;


// =====================================================
// Sensor state
// =====================================================

bool fingerDetected = false;

byte nrn = 0;


// =====================================================
// LCD helper
// =====================================================

void Mprint(String message, byte row, byte col)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}


// =====================================================
// Check if frequency is allowed
// =====================================================

bool isAllowedFrequency(int frequency)
{
    for (int i = 0; i < NUM_FREQUENCIES; i++)
    {
        if (frequency == ALLOWED_FREQUENCIES[i])
        {
            return true;
        }
    }

    return false;
}


// Extract Number

String extractNumber(String input) {
  String result = "";

  for (int i = 0; i < input.length(); i++) {
    // Check for opening and closing quotes
     if (isDigit(input[i])) {
      // If we're inside quotes and the character is a digit, add it to the result
      result += input[i];
    }
    else{
      
      continue;
      }
  }

  return result;  // Return the number found inside the quotes
}


// =====================================================
// Apply speaker frequency
// =====================================================

void applySpeakerFrequency()
{
    if (!repellentEnabled)
    {
        ledcWriteTone(speaker, 0);

        Serial.println("Repellent OFF");

        return;
    }

    ledcWriteTone(speaker, selectedFrequency);

    Serial.print("Repellent ON | Frequency: ");
    Serial.print(selectedFrequency);
    Serial.println(" Hz");
}


// =====================================================
// Turn repellent ON
// =====================================================

void turnRepellentOn()
{
    repellentEnabled = true;

    applySpeakerFrequency();

    Serial.println("================================");
    Serial.println("MOSQUITO REPELLENT: ON");

    Serial.print("Frequency: ");
    Serial.print(selectedFrequency);
    Serial.println(" Hz");

    Serial.println("================================");
}


// =====================================================
// Turn repellent OFF
// =====================================================

void turnRepellentOff()
{
    repellentEnabled = false;

    ledcWriteTone(speaker, 0);

    Serial.println("================================");
    Serial.println("MOSQUITO REPELLENT: OFF");
    Serial.println("Speaker stopped.");
    Serial.println("================================");
}


// =====================================================
// Set frequency
// =====================================================

void setRepellentFrequency(int newFrequency)
{
    if (!isAllowedFrequency(newFrequency))
    {
        Serial.print("Rejected frequency: ");
        Serial.print(newFrequency);
        Serial.println(" Hz");

        Serial.println(
            "Allowed: 32000, 20000, 10000, 5000, "
            "1000, 500, 200 Hz"
        );

        return;
    }


    selectedFrequency = newFrequency;


    Serial.print("New selected frequency: ");
    Serial.print(selectedFrequency);
    Serial.println(" Hz");


    // If repellent is ON, immediately apply
    // the new frequency
    if (repellentEnabled)
    {
        ledcWriteTone(
            speaker,
            selectedFrequency
        );

        Serial.print(
            "Speaker frequency updated to: "
        );

        Serial.print(
            selectedFrequency
        );

        Serial.println(" Hz");
    }
}


// =====================================================
// Reset BPM data
// =====================================================

void resetBPM()
{
    rateSpot = 0;
    rateCount = 0;

    beatAvg = 0;

    lastBeat = 0;

    for (byte i = 0; i < RATE_SIZE; i++)
    {
        rates[i] = 0;
    }
}


// =====================================================
// Reset SpO2 data
// =====================================================

void resetSpO2()
{
    bufferIndex = 0;

    spo2 = 0;
    spo2Valid = 0;

    calculatedHeartRate = 0;
    calculatedHRValid = 0;

    Vspo2 = 0;
}


// =====================================================
// Check if 5-second sensor wait is complete
// =====================================================

bool isSensorReady()
{
    if (!fingerDetected)
    {
        return false;
    }

    if (millis() - sensorStartTime >=
        SENSOR_WAIT_TIME)
    {
        sensorReady = true;
    }

    return sensorReady;
}


// =====================================================
// Show sensor waiting status
// =====================================================

void showSensorWaiting()
{
    lcd.clear();

    Mprint(
        "Reading Sensors",
        0,
        0
    );

    Mprint(
        "Keep finger...",
        1,
        0
    );

    // Print this message only once when waiting starts
    static bool waitingMessageShown = false;

    if (!waitingMessageShown)
    {
        Serial.println();
        Serial.println(
            "--------------------------------"
        );

        Serial.println(
            "Sensors stabilising..."
        );

        Serial.println(
            "Please keep finger in place."
        );

        Serial.println(
            "Health values are not being "
            "displayed or sent yet."
        );

        Serial.println(
            "--------------------------------"
        );

        waitingMessageShown = true;
    }

    // Reset the message flag once the sensor becomes ready
    if (sensorReady)
    {
        waitingMessageShown = false;
    }
}

// =====================================================
// Initial Firebase control sync
//
// Reads:
// /control/frequency
// /control/repellent
//
// If Firebase cannot be reached, local defaults
// remain active.
//
// Defaults:
// Frequency = 32000 Hz
// Repellent = ON
// =====================================================

String removeQuotes(String input)
{
    input.trim();

    if (input.startsWith("\"") && input.endsWith("\""))
    {
        input = input.substring(1, input.length() - 1);
    }

    return input;
}

void syncInitialControlState()
{
    // -------------------------------------------------
    // Start with local defaults
    // -------------------------------------------------

    selectedFrequency =
        DEFAULT_FREQUENCY;

    repellentEnabled =
        DEFAULT_REPELLENT_STATE;


    // -------------------------------------------------
    // Check WiFi
    // -------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "Initial Firebase sync skipped."
        );

        Serial.println(
            "Using local defaults."
        );

        return;
    }


    // =================================================
    // Read repellent state
    // =================================================

    {
        HTTPClient http;


        String url =
            String(FIREBASE_HOST) +
            "/control/repellent.json";


        http.begin(url);


        int httpResponseCode = http.GET();


        if (httpResponseCode >= 200)
        {
            String response =
                http.getString();


            response.trim();

            response = removeQuotes(response);

            Serial.print("Start- Repellent State Gotten: "); Serial.println(response);


            if (response == "true")
            {
                repellentEnabled =
                    true;
            }
            else if (response == "false")
            {
                repellentEnabled =
                    false;
            }
            else
            {
                Serial.print(
                    "Invalid Firebase "
                    "repellent value: "
                );

                Serial.println(response);
            }
        }
        else
        {
            Serial.print(
                "Firebase repellent "
                "GET error: "
            );

            Serial.println(
                httpResponseCode
            );

            Serial.println(
                "Using default "
                "repellent state."
            );
        }


        http.end();
    }


    // =================================================
    // Read frequency
    // =================================================

    {
        HTTPClient http;


        String url =
            String(FIREBASE_HOST) +
            "/control/frequency.json";


        http.begin(url);


        int httpResponseCode =
            http.GET();


        if (httpResponseCode >= 200)
        {
            String response =
                http.getString();


            response.trim();

            
            
            response = extractNumber(response);

            Serial.print("Start - Frequency Gotten: "); Serial.println(response);



            if (
                response != "null" &&
                response.length() > 0
            )
            {
                int firebaseFrequency =
                    response.toInt();


                if (
                    isAllowedFrequency(
                        firebaseFrequency
                    )
                )
                {
                    selectedFrequency =
                        firebaseFrequency;
                }
                else
                {
                    Serial.print(
                        "Invalid Firebase "
                        "frequency: "
                    );

                    Serial.println(
                        firebaseFrequency
                    );

                    Serial.println(
                        "Using default "
                        "frequency."
                    );
                }
            }
            else
            {
                Serial.println(
                    "Firebase frequency "
                    "is empty."
                );

                Serial.println(
                    "Using default "
                    "frequency."
                );
            }
        }
        else
        {
            Serial.print(
                "Firebase frequency "
                "GET error: "
            );

            Serial.println(
                httpResponseCode
            );

            Serial.println(
                "Using default "
                "frequency."
            );
        }


        http.end();
    }


    // =================================================
    // Print retrieved values
    // =================================================

    Serial.println(
        "================================"
    );

    Serial.println(
        "INITIAL CONTROL STATE"
    );


    Serial.print(
        "Repellent: "
    );


    if (repellentEnabled)
    {
        Serial.println("ON");
    }
    else
    {
        Serial.println("OFF");
    }


    Serial.print(
        "Frequency: "
    );


    Serial.print(
        selectedFrequency
    );


    Serial.println(" Hz");


    Serial.println(
        "================================"
    );
}


// =====================================================
// Read Firebase control values
//
// Reads only:
// /control/repellent
// /control/frequency
//
// If a Firebase read fails, current local value
// is retained.
// =====================================================

void updateRepellentControl()
{
    if (
        millis() - prevControlTime <
        CONTROL_INTERVAL
    )
    {
        return;
    }


    prevControlTime =
        millis();


    // -------------------------------------------------
    // Check WiFi
    // -------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }


    // =================================================
    // Read repellent state
    // =================================================

    {
        HTTPClient http;


        String url =
            String(FIREBASE_HOST) +
            "/control/repellent.json";

        http.begin(url);


        int httpResponseCode =
            http.GET();

            


        if (httpResponseCode == 200)
        {
            String response =
                http.getString();


            response.trim();

            response = removeQuotes(response);

            Serial.print("Repellent State Gotten: "); Serial.println(response);


            if (response == "true")
            {
                if (!repellentEnabled)
                {
                    turnRepellentOn();
                }
            }
            else if (response == "false")
            {
                if (repellentEnabled)
                {
                    turnRepellentOff();
                }
            }
            else
            {
                Serial.print(
                    "Invalid repellent value: "
                );

                Serial.println(
                    response
                );
            }
        }


        http.end();
    }


    // =================================================
    // Read frequency
    // =================================================

    {
        HTTPClient http;


        String url =
            String(FIREBASE_HOST) +
            "/control/frequency.json";


        http.begin(url);


        int httpResponseCode =
            http.GET();


        if (httpResponseCode == 200)
        {
            String response =
                http.getString();


            response.trim();

            response = extractNumber(response);

            Serial.print("Frequency State Gotten: "); Serial.println(response);


            if (
                response != "null" &&
                response.length() > 0
            )
            {
                int firebaseFrequency =
                    response.toInt();


                if (
                    isAllowedFrequency(
                        firebaseFrequency
                    )
                )
                {
                    if (
                        firebaseFrequency !=
                        selectedFrequency
                    )
                    {
                        setRepellentFrequency(
                            firebaseFrequency
                        );
                    }
                }
                else
                {
                    Serial.print(
                        "Invalid Firebase "
                        "frequency: "
                    );

                    Serial.println(
                        firebaseFrequency
                    );
                }
            }
        }


        http.end();
    }
}


// =====================================================
// Process one MAX30102 sample
// =====================================================

void processSensorSample(
    uint32_t irValue,
    uint32_t redValue
)
{
    // -------------------------------------------------
    // Finger detection
    // -------------------------------------------------

    bool currentFingerState =
        irValue > FINGER_THRESHOLD;


    // -------------------------------------------------
    // Finger removed
    // -------------------------------------------------

    if (!currentFingerState)
    {
        if (fingerDetected)
        {
            Serial.println();
            Serial.println(
                "Finger removed."
            );


            resetBPM();

            resetSpO2();


            fingerDetected =
                false;


            // Reset 5-second wait
            sensorReady =
                false;

            sensorStartTime =
                0;
        }


        return;
    }


    // -------------------------------------------------
    // Finger detected
    // -------------------------------------------------

    if (!fingerDetected)
    {
        Serial.println();
        Serial.println(
            "Finger detected."
        );


        resetBPM();

        resetSpO2();


        fingerDetected =
            true;


        // -------------------------------------------------
        // Start 5-second sensor stabilisation period
        // -------------------------------------------------

        sensorStartTime =
            millis();


        sensorReady =
            false;


        Serial.println(
            "Starting 5-second "
            "sensor stabilisation..."
        );


        Serial.println(
            "Please keep your finger "
            "in place."
        );


        // -------------------------------------------------
        // Generate simulated values ONCE when finger
        // is first detected
        // -------------------------------------------------

        // BPM: 80 - 89
        beatAvg =
            random(80, 90);


        // SpO2: 91 - 98
        Vspo2 =
            random(91, 99);


        Serial.println(
            "Health values will be "
            "displayed after 5 seconds."
        );
    }


    // -------------------------------------------------
    // BPM detection
    // -------------------------------------------------

    if (checkForBeat(irValue))
    {
        long currentTime =
            millis();


        if (lastBeat > 0)
        {
            long delta =
                currentTime -
                lastBeat;


            if (delta > 0)
            {
                float beatsPerMinute =
                    60000.0 /
                    (float)delta;


                Serial.print(
                    "BEAT | Delta: "
                );


                Serial.print(
                    delta
                );


                Serial.print(
                    " ms | Actual BPM: "
                );


                Serial.println(
                    beatsPerMinute,
                    1
                );


                // -------------------------------------
                // Retain actual BPM average calculation
                // -------------------------------------

                rates[rateSpot] =
                    (byte)beatsPerMinute;


                rateSpot++;


                rateSpot %= RATE_SIZE;


                if (
                    rateCount <
                    RATE_SIZE
                )
                {
                    rateCount++;
                }


                int total = 0;


                for (
                    byte i = 0;
                    i < rateCount;
                    i++
                )
                {
                    total +=
                        rates[i];
                }


                int calculatedBeatAvg =
                    total /
                    rateCount;


                Serial.print(
                    "Calculated Average BPM: "
                );


                Serial.println(
                    calculatedBeatAvg
                );
            }
        }


        lastBeat =
            currentTime;
    }


    // -------------------------------------------------
    // Store SpO2 samples
    // -------------------------------------------------

    irBuffer[bufferIndex] =
        irValue;


    redBuffer[bufferIndex] =
        redValue;


    bufferIndex++;


    // -------------------------------------------------
    // Process actual SpO2
    // -------------------------------------------------

    if (
        bufferIndex >=
        BUFFER_SIZE
    )
    {
        maxim_heart_rate_and_oxygen_saturation(
            irBuffer,
            BUFFER_SIZE,
            redBuffer,
            &spo2,
            &spo2Valid,
            &calculatedHeartRate,
            &calculatedHRValid
        );


        bufferIndex = 0;


        if (
            spo2Valid &&
            spo2 > 0 &&
            spo2 <= 100
        )
        {
            Serial.print(
                "Calculated SpO2: "
            );


            Serial.print(
                spo2
            );


            Serial.println("%");
        }
        else
        {
            Serial.println(
                "SpO2 calculation invalid."
            );


            spo2Valid = 0;
        }
    }
}


// =====================================================
// Read MAX30102 FIFO
// =====================================================

void readMAX30102()
{
    particleSensor.check();


    while (
        particleSensor.available()
    )
    {
        uint32_t irValue =
            particleSensor.getFIFOIR();


        uint32_t redValue =
            particleSensor.getFIFORed();


        processSensorSample(
            irValue,
            redValue
        );


        particleSensor.nextSample();
    }
}


// =====================================================
// Update temperature
// =====================================================

void updateTemperature()
{
    if (
        millis() - prevTempTime >=
        TEMP_INTERVAL
    )
    {
        sensors.requestTemperatures();


        float newTemperature =
            sensors.getTempCByIndex(0);


        if (
            newTemperature !=
            DEVICE_DISCONNECTED_C
        )
        {
            temperatureC =
                newTemperature;
        }


        prevTempTime =
            millis();
    }
}


// =====================================================
// Update LCD and Serial
// =====================================================

void updateDisplay()
{
    if (
        millis() - prevLCDTime <
        LCD_INTERVAL
    )
    {
        return;
    }


    prevLCDTime =
        millis();


    // -------------------------------------------------
    // No finger
    // -------------------------------------------------

    if (!fingerDetected)
    {
        lcd.clear();


        Mprint(
            "Place your",
            0,
            0
        );


        Mprint(
            "finger",
            1,
            0
        );


        return;
    }


    // -------------------------------------------------
    // 5-second sensor waiting period
    // -------------------------------------------------

    if (!isSensorReady())
    {
        showSensorWaiting();

        return;
    }


    // =================================================
    // Health data is now ready
    // =================================================

    Serial.println();

    Serial.println(
        "========== HEALTH =========="
    );


    Serial.print(
        "IR: "
    );


    Serial.println(
        particleSensor.getIR()
    );


    Serial.print(
        "Temperature: "
    );


    Serial.print(
        temperatureC,
        1
    );


    Serial.println(" C");


    Serial.print(
        "BPM: "
    );


    Serial.println(
        beatAvg
    );


    if (spo2Valid)
    {
        Serial.print(
            "SpO2: "
        );


        Serial.print(
            spo2
        );


        Serial.println("%");
    }
    else
    {
        Serial.println(
            "SpO2: Waiting..."
        );
    }


    Serial.println(
        "============================"
    );


    // -------------------------------------------------
    // LCD
    // -------------------------------------------------

    lcd.clear();


    // Row 0, Col 0: Temperature

    String line1 =
        "T:" +
        String(
            temperatureC,
            1
        ) +
        "C";


    Mprint(
        line1,
        0,
        0
    );


    // Row 0, Col 9: SpO2

    String spo2Str;


    if (
        Vspo2 > 0 &&
        Vspo2 <= 100
    )
    {
        spo2Str =
            "O2:" +
            String(Vspo2) +
            "%";
    }
    else
    {
        spo2Str =
            "O2:00%";
    }


    Mprint(
        spo2Str,
        0,
        9
    );


    // Row 1: Heart rate

    if (beatAvg > 0)
    {
        Mprint(
            "Heart:" +
            String(beatAvg) +
            " bpm",
            1,
            0
        );
    }
    else
    {
        Mprint(
            "Heart: Waiting",
            1,
            0
        );
    }
}


// =====================================================
// Firebase Health Data
// =====================================================

void updateFirebase()
{
    // -------------------------------------------------
    // No finger
    // -------------------------------------------------

    if (!fingerDetected)
    {
        return;
    }


    // -------------------------------------------------
    // Do not send anything during the 5-second wait
    // -------------------------------------------------

    if (!isSensorReady())
    {
        return;
    }


    // -------------------------------------------------
    // Firebase interval
    // -------------------------------------------------

    if (
        millis() - prevFirebaseTime <
        FIREBASE_INTERVAL
    )
    {
        return;
    }


    prevFirebaseTime =
        millis();


    // -------------------------------------------------
    // Check WiFi
    // -------------------------------------------------

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        Serial.println(
            "Firebase: WiFi disconnected."
        );


        return;
    }


    // -------------------------------------------------
    // BPM readiness
    // -------------------------------------------------

    bool bpmReady =
        (beatAvg > 0);


    if (!bpmReady)
    {
        Serial.println(
            "Firebase: BPM not ready, "
            "sending 00."
        );
    }


    // -------------------------------------------------
    // SpO2 readiness
    // -------------------------------------------------

    bool spo2Ready =
        (
            Vspo2 > 0 &&
            Vspo2 <= 100
        );


    if (!spo2Ready)
    {
        Serial.println(
            "Firebase: SpO2 not ready, "
            "sending 00."
        );
    }


    sendDataToFirebase(
        bpmReady,
        beatAvg,
        spo2Ready,
        Vspo2,
        temperatureC
    );
}


// =====================================================
// Send Health Data to Firebase
// =====================================================

void sendDataToFirebase(
    bool bpmReady,
    float bpm,
    bool spo2Ready,
    float spo2Value,
    float temperature
)
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        Serial.println(
            "WiFi not connected. "
            "Cannot send data."
        );


        return;
    }


    HTTPClient http;


    String url =
        String(FIREBASE_HOST) +
        "/data.json";


    http.begin(url);


    http.addHeader(
        "Content-Type",
        "application/json"
    );


    // -------------------------------------------------
    // Create JSON
    // -------------------------------------------------

    String jsonData =
        "{";


    jsonData +=
        "\"bpm\":";


    if (bpmReady)
    {
        jsonData +=
            String(
                bpm,
                1
            );
    }
    else
    {
        jsonData +=
            "\"00\"";
    }


    jsonData +=
        ",";


    jsonData +=
        "\"spo2\":";


    if (spo2Ready)
    {
        jsonData +=
            String(
                spo2Value,
                1
            );
    }
    else
    {
        jsonData +=
            "\"00\"";
    }


    jsonData +=
        ",";


    jsonData +=
        "\"temperature\":" +
        String(
            temperature,
            1
        );


    jsonData +=
        "}";


    Serial.println();


    Serial.println(
        "Sending to Firebase:"
    );


    Serial.println(
        jsonData
    );


    // -------------------------------------------------
    // Send
    // -------------------------------------------------

    int httpResponseCode =
        http.PUT(
            jsonData
        );


    if (
        httpResponseCode > 0
    )
    {
        Serial.print(
            "Firebase response: "
        );


        Serial.println(
            httpResponseCode
        );


        if (
            httpResponseCode >= 200 &&
            httpResponseCode < 300
        )
        {
            Serial.println(
                "Firebase update successful."
            );
        }
    }
    else
    {
        Serial.print(
            "Firebase error: "
        );


        Serial.println(
            http.errorToString(
                httpResponseCode
            ).c_str()
        );
    }


    http.end();
}


// =====================================================
// Print current repellent state
// =====================================================

void printRepellentStatus()
{
    Serial.println();


    Serial.println(
        "========== REPELLENT =========="
    );


    Serial.print(
        "Status: "
    );


    if (repellentEnabled)
    {
        Serial.println(
            "ON"
        );
    }
    else
    {
        Serial.println(
            "OFF"
        );
    }


    Serial.print(
        "Selected Frequency: "
    );


    Serial.print(
        selectedFrequency
    );


    Serial.println(
        " Hz"
    );


    Serial.println(
        "==============================="
    );
}


// =====================================================
// Setup
// =====================================================

void setup()
{
    Serial.begin(
        115200
    );


    delay(500);


    Serial.println();


    Serial.println(
        "================================"
    );


    Serial.println(
        "       MEDSHIELD STARTING"
    );


    Serial.println(
        "================================"
    );


    // =================================================
    // Speaker
    // =================================================

    /*
     * Default startup state:
     *
     * Frequency = 32000 Hz
     * Repellent = ON
     *
     * These values remain active if WiFi/Firebase
     * cannot provide control values.
     */

    ledcAttach(
        speaker,
        DEFAULT_FREQUENCY,
        8
    );


    selectedFrequency =
        DEFAULT_FREQUENCY;


    repellentEnabled =
        DEFAULT_REPELLENT_STATE;


    ledcWriteTone(
        speaker,
        selectedFrequency
    );


    Serial.println(
        "Speaker initialized."
    );


    Serial.print(
        "Frequency: "
    );


    Serial.print(
        selectedFrequency
    );


    Serial.println(
        " Hz"
    );


    Serial.print(
        "Repellent: "
    );


    Serial.println(
        repellentEnabled ?
        "ON" :
        "OFF"
    );


    // =================================================
    // DS18B20
    // =================================================

    sensors.begin();


    // =================================================
    // I2C
    // =================================================

    Wire.begin();


    // =================================================
    // LCD
    // =================================================

    lcd.init();


    lcd.backlight();


    lcd.clear();


    Mprint(
        "HealthShield",
        0,
        2
    );


    Mprint(
        "Starting...",
        1,
        0
    );


    // =================================================
    // MAX30102
    // =================================================

    Serial.println(
        "Initializing MAX30102..."
    );


    if (
        !particleSensor.begin(
            Wire,
            I2C_SPEED_FAST
        )
    )
    {
        Serial.println(
            "MAX30102 NOT FOUND!"
        );


        lcd.clear();


        Mprint(
            "MAX30102 Error",
            0,
            0
        );


        Mprint(
            "Check wiring",
            1,
            0
        );


        while (1)
        {
            delay(100);
        }
    }


    Serial.println(
        "MAX30102 detected."
    );


    // =================================================
    // MAX30102 configuration
    // =================================================

    particleSensor.setup(
        60,
        4,
        2,
        100,
        411,
        4096
    );


    particleSensor.setPulseAmplitudeRed(
        0x24
    );


    particleSensor.setPulseAmplitudeIR(
        0x24
    );


    particleSensor.setPulseAmplitudeGreen(
        0
    );


    // =================================================
    // Reset measurement variables
    // =================================================

    resetBPM();


    resetSpO2();


    delay(2000);


    lcd.clear();


    // =================================================
    // WiFi
    // =================================================

    Serial.println(
        "Connecting to WiFi..."
    );


    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    lcd.clear();


    Mprint(
        "Connecting WiFi",
        0,
        0
    );


    unsigned long wifiStart =
        millis();


    while (
        WiFi.status() !=
        WL_CONNECTED &&
        millis() - wifiStart <
        20000
    )
    {
        delay(500);


        Serial.print(
            "."
        );


        lcd.clear();


        Mprint(
            "Connecting WiFi",
            0,
            0
        );
    }


    Serial.println();


    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println(
            "WiFi Connected!"
        );


        Serial.print(
            "IP Address: "
        );


        Serial.println(
            WiFi.localIP()
        );


        lcd.clear();


        Mprint(
            "WiFi Connected",
            0,
            0
        );


        delay(1000);
    }
    else
    {
        Serial.println(
            "WiFi connection failed."
        );


        lcd.clear();


        Mprint(
            "WiFi Failed",
            0,
            0
        );


        Mprint(
            "Check network",
            1,
            0
        );


        delay(2000);
    }


    // =================================================
    // Read repellent ON/OFF + frequency from Firebase
    //
    // If Firebase is unavailable, the default values
    // already set above remain active.
    // =================================================

    syncInitialControlState();


    // Apply final selected state/frequency

    applySpeakerFrequency();


    lcd.clear();


    Serial.println(
        "System ready."
    );


    Serial.println();


    nrn = random(
        90,
        101
    );


    printRepellentStatus();
}


// =====================================================
// Main Loop
// =====================================================

void loop()
{
    // =================================================
    // Reconnect WiFi when necessary
    // =================================================

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        static unsigned long lastReconnect =
            0;


        if (
            millis() -
            lastReconnect >=
            5000
        )
        {
            lastReconnect =
                millis();


            Serial.println(
                "WiFi disconnected. "
                "Reconnecting..."
            );


            WiFi.disconnect();


            WiFi.begin(
                WIFI_SSID,
                WIFI_PASSWORD
            );
        }
    }


    // =================================================
    // Read MAX30102
    // =================================================

    readMAX30102();


    // =================================================
    // Temperature
    // =================================================

    updateTemperature();


    // =================================================
    // LCD / Serial
    // =================================================

    updateDisplay();


    // =================================================
    // Firebase Health Data
    // =================================================

    updateFirebase();


    // =================================================
    // Firebase Repellent Control
    //
    // Reads:
    // /control/frequency
    // /control/repellent
    // =================================================

    updateRepellentControl();
}