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

// Default frequency
const int DEFAULT_FREQUENCY = 32000;

// Allowed frequencies
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

// Current selected frequency
int selectedFrequency = DEFAULT_FREQUENCY;

// Repellent state
bool repellentEnabled = true;

// Firebase control polling
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

// =====================================================
// Apply speaker frequency
// =====================================================

void applySpeakerFrequency()
{
    if (!repellentEnabled)
    {
        // Repellent OFF
        ledcWriteTone(speaker, 0);

        Serial.println("Repellent OFF");
        return;
    }

    // Repellent ON
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
    // Reject unsupported frequencies
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

    // Frequency can only be changed while OFF
    if (repellentEnabled)
    {
        Serial.println(
            "Frequency change ignored because "
            "repellent is ON."
        );

        return;
    }

    selectedFrequency = newFrequency;

    Serial.print("New selected frequency: ");
    Serial.print(selectedFrequency);
    Serial.println(" Hz");

    Serial.println(
        "Turn repellent ON to activate this frequency."
    );
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
// Read Firebase control values
// =====================================================

void updateRepellentControl()
{
    if (millis() - prevControlTime < CONTROL_INTERVAL)
    {
        return;
    }

    prevControlTime = millis();

    // -------------------------------------------------
    // Check WiFi
    // -------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "Control: WiFi disconnected."
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

        http.addHeader(
            "Content-Type",
            "application/json"
        );

        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String response = http.getString();

            response.trim();

            bool firebaseRepellent;

            if (response == "true")
            {
                firebaseRepellent = true;
            }
            else if (response == "false")
            {
                firebaseRepellent = false;
            }
            else
            {
                Serial.print(
                    "Invalid repellent value: "
                );

                Serial.println(response);

                http.end();

                return;
            }

            // -----------------------------------------
            // Only react if state changed
            // -----------------------------------------

            if (firebaseRepellent != repellentEnabled)
            {
                if (firebaseRepellent)
                {
                    turnRepellentOn();
                }
                else
                {
                    turnRepellentOff();
                }
            }
        }
        else
        {
            Serial.print(
                "Control repellent GET error: "
            );

            Serial.println(httpResponseCode);
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

        http.addHeader(
            "Content-Type",
            "application/json"
        );

        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String response = http.getString();

            response.trim();

            // Firebase returns something like:
            //
            // 32000
            //
            // Convert to integer

            int firebaseFrequency =
                response.toInt();

            if (isAllowedFrequency(firebaseFrequency))
            {
                if (firebaseFrequency != selectedFrequency)
                {
                    setRepellentFrequency(
                        firebaseFrequency
                    );
                }
            }
            else
            {
                Serial.print(
                    "Invalid Firebase frequency: "
                );

                Serial.println(firebaseFrequency);
            }
        }
        else
        {
            Serial.print(
                "Control frequency GET error: "
            );

            Serial.println(httpResponseCode);
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
            Serial.println("Finger removed.");

            resetBPM();
            resetSpO2();

            fingerDetected = false;
        }

        return;
    }

    // -------------------------------------------------
    // Finger detected
    // -------------------------------------------------

    if (!fingerDetected)
    {
        Serial.println();
        Serial.println("Finger detected.");

        resetBPM();
        resetSpO2();

        fingerDetected = true;
    }

    // -------------------------------------------------
    // BPM detection
    // -------------------------------------------------

    if (checkForBeat(irValue))
    {
        long currentTime = millis();

        if (lastBeat > 0)
        {
            long delta =
                currentTime - lastBeat;

            if (delta > 0)
            {
                float beatsPerMinute =
                    60000.0 /
                    (float)delta;

                Serial.print(
                    "BEAT | Delta: "
                );

                Serial.print(delta);

                Serial.print(
                    " ms | BPM: "
                );

                Serial.println(
                    beatsPerMinute,
                    1
                );

                // -------------------------------------
                // Accept realistic heart-rate range
                // -------------------------------------

                if (beatsPerMinute >= 40 &&
                    beatsPerMinute <= 180)
                {
                    rates[rateSpot] =
                        (byte)beatsPerMinute;

                    rateSpot++;

                    rateSpot %= RATE_SIZE;

                    if (rateCount < RATE_SIZE)
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
                        total += rates[i];
                    }

                    beatAvg =
                        total /
                        rateCount;

                    // Original code behavior
                    if (beatAvg < 30)
                    {
                        beatAvg =
                            beatAvg +
                            random(40, 80);
                    }
                    else if (beatAvg < 40)
                    {
                        beatAvg =
                            beatAvg +
                            random(30, 70);
                    }
                    else if (beatAvg < 60)
                    {
                        beatAvg =
                            beatAvg +
                            random(10, 60);
                    }

                    Serial.print(
                        "Average BPM: "
                    );

                    Serial.println(beatAvg);
                }
            }
        }

        lastBeat = currentTime;
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
    // Process SpO2
    // -------------------------------------------------

    if (bufferIndex >= BUFFER_SIZE)
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

        if (spo2Valid &&
            spo2 > 0 &&
            spo2 <= 100)
        {
            Vspo2 = spo2;

            Serial.print(
                "Valid SpO2: "
            );

            Serial.print(spo2);

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

    while (particleSensor.available())
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
    if (millis() - prevTempTime >=
        TEMP_INTERVAL)
    {
        sensors.requestTemperatures();

        float newTemperature =
            sensors.getTempCByIndex(0);

        if (newTemperature !=
            DEVICE_DISCONNECTED_C)
        {
            temperatureC =
                newTemperature;
        }

        prevTempTime = millis();
    }
}

// =====================================================
// Update LCD and Serial
// =====================================================

void updateDisplay()
{
    if (millis() - prevLCDTime <
        LCD_INTERVAL)
    {
        return;
    }

    prevLCDTime = millis();

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

        Serial.println();
        Serial.println(
            "No finger detected."
        );

        return;
    }

    // -------------------------------------------------
    // Finger detected
    // -------------------------------------------------

    Serial.println();
    Serial.println(
        "========== HEALTH =========="
    );

    Serial.print("IR: ");

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

    Serial.print("BPM: ");

    Serial.println(beatAvg);

    if (spo2Valid)
    {
        Serial.print("SpO2: ");

        Serial.print(spo2);

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

    String line1 =
        "T:" +
        String(
            temperatureC,
            1
        ) +
        "C ";

    if (spo2Valid)
    {
        line1 +=
            "O2:" +
            String(spo2) +
            "%";
    }

    Mprint(
        line1,
        0,
        0
    );

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

        Serial.print(
            "SHOW BPM: "
        );

        Serial.println(beatAvg);
    }
}

// =====================================================
// Firebase Health Data
// =====================================================

void updateFirebase()
{
    // -------------------------------------------------
    // Only upload when finger is detected
    // -------------------------------------------------

    if (!fingerDetected)
    {
        return;
    }

    // -------------------------------------------------
    // Only upload when BPM is valid
    // -------------------------------------------------

    if (beatAvg <= 0)
    {
        Serial.println(
            "Firebase: Waiting for BPM..."
        );

        return;
    }

    if (millis() - prevFirebaseTime <
        FIREBASE_INTERVAL)
    {
        return;
    }

    prevFirebaseTime = millis();

    // -------------------------------------------------
    // Check WiFi
    // -------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "Firebase: WiFi disconnected."
        );

        return;
    }

    // -------------------------------------------------
    // Use latest valid SpO2 value
    // -------------------------------------------------

    float uploadSpO2 = 0;

    if (spo2Valid &&
        spo2 > 0 &&
        spo2 <= 100)
    {
        uploadSpO2 = Vspo2;
    }
    else
    {
        uploadSpO2 = 0;
    }

    sendDataToFirebase(
        beatAvg,
        uploadSpO2,
        temperatureC
    );
}

// =====================================================
// Send Health Data to Firebase
// =====================================================

void sendDataToFirebase(
    float bpm,
    float spo2Value,
    float temperature
)
{
    if (WiFi.status() != WL_CONNECTED)
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

    String jsonData = "{";

    jsonData +=
        "\"bpm\":" +
        String(bpm, 1) +
        ",";

    jsonData +=
        "\"spo2\":" +
        String(spo2Value, 1) +
        ",";

    jsonData +=
        "\"temperature\":" +
        String(temperature, 1);

    jsonData += "}";

    Serial.println();

    Serial.println(
        "Sending to Firebase:"
    );

    Serial.println(jsonData);

    // -------------------------------------------------
    // Send
    // -------------------------------------------------

    int httpResponseCode =
        http.PUT(jsonData);

    if (httpResponseCode > 0)
    {
        Serial.print(
            "Firebase response: "
        );

        Serial.println(
            httpResponseCode
        );

        if (httpResponseCode >= 200 &&
            httpResponseCode < 300)
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
        Serial.println("ON");
    }
    else
    {
        Serial.println("OFF");
    }

    Serial.print(
        "Selected Frequency: "
    );

    Serial.print(
        selectedFrequency
    );

    Serial.println(" Hz");

    Serial.println(
        "==============================="
    );
}

// =====================================================
// Setup
// =====================================================

void setup()
{
    Serial.begin(115200);

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
     * Configure GPIO25 for PWM.
     *
     * IMPORTANT:
     * The speaker is no longer permanently forced
     * to 32 kHz here.
     *
     * The repellent starts ON at 32 kHz to preserve
     * the original device behavior.
     *
     * Firebase can subsequently change the state.
     */

    ledcAttach(
        speaker,
        DEFAULT_FREQUENCY,
        8
    );

    selectedFrequency =
        DEFAULT_FREQUENCY;

    repellentEnabled = true;

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

    Serial.println(" Hz");

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

    if (!particleSensor.begin(
            Wire,
            I2C_SPEED_FAST
        ))
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
        WiFi.status() != WL_CONNECTED &&
        millis() - wifiStart < 20000
    )
    {
        delay(500);

        Serial.print(".");

        lcd.clear();

        Mprint(
            "Connecting WiFi",
            0,
            0
        );
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
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

    if (WiFi.status() != WL_CONNECTED)
    {
        static unsigned long lastReconnect = 0;

        if (
            millis() -
            lastReconnect >= 5000
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
    // =================================================

    updateRepellentControl();

    // =================================================
    // Debug BPM
    // =================================================

    Serial.print(
        "Average BPM: "
    );

    Serial.println(
        beatAvg
    );
}