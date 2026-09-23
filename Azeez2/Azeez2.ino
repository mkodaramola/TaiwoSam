/*
  PAYGO Solar Energy Meter
  ESP32 + PZEM-004T + 16x2 I2C LCD + 2 relays

  Base: Azeez.ino
  Communication:
    GET  /api/devices/{deviceId}
    POST /api/devices/{deviceId}/consumption

  The firmware keeps a local pending-consumption ledger in ESP32 NVS
  (Preferences). Every consumption report has a persistent reportId.
  If the HTTP response is lost after the backend commits, the same
  reportId is retried and the backend must return the original result
  instead of deducting the energy again.

  IMPORTANT:
    - Do NOT reset the PZEM cumulative energy counter during normal use.
    - Set API_BASE_URL to the deployed backend URL.
    - Install:
        1. LiquidCrystal I2C by Frank de Brabander
        2. ArduinoJson
        3. ESP32 Arduino core

  Hardware from the supplied schematic:
    PZEM TX -> GPIO16 (RX2)
    PZEM RX -> GPIO17 (TX2)
    LCD SDA -> GPIO21
    LCD SCL -> GPIO22
    Relay control -> GPIO5
    Buzzer -> GPIO23
    Confirm button -> GPIO4

  Relay circuit is an NPN low-side driver, so GPIO5 HIGH energizes
  both relay coils in the supplied schematic.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// USER CONFIGURATION
// ============================================================

const char* WIFI_SSID     = "Premauda_STARLINK2";
const char* WIFI_PASSWORD = "Technologies";

// Example:
// const char* API_BASE_URL = "https://paygo-backend.onrender.com";
const char* API_BASE_URL = "https://YOUR-BACKEND-DOMAIN";

const char* DEVICE_ID = "DEVICE-001";

// Optional device key. Leave empty until the backend endpoint is
// protected with an X-Device-Key check.
const char* DEVICE_API_KEY = "";

// ============================================================
// BALANCE SOURCE
// ============================================================
// TEST mode is used while the backend API is unavailable.
// The simulated balance starts at 100 kWh on the first test run.
// Consumption is still handled by the same pending-energy ledger
// used by the real API path. Therefore: available balance =
// purchased balance - locally measured/unreported consumption.
//
// When the backend is ready, change this to false and uncomment
// the API balance function and its call in getCurrentEnergyBalance().

#define USE_TEST_BALANCE 1
const double TEST_INITIAL_BALANCE_KWH = 100.0;

// ============================================================
// PINS
// ============================================================

#define PZEM_RX_PIN     16
#define PZEM_TX_PIN     17
#define I2C_SDA         21
#define I2C_SCL         22
#define RELAY_PIN       5
#define BUZZER_PIN      23
#define CONFIRM_PIN     4

// ============================================================
// PZEM CONFIGURATION
// ============================================================

#define PZEM_ADDR       0xF8
#define PZEM_BAUD       9600
#define PZEM_TRIES      3

const float VOLT_CAL = 1.000f;

// ============================================================
// LCD
// ============================================================

#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C* lcd = nullptr;

// ============================================================
// TIMING
// ============================================================

const unsigned long READ_INTERVAL_MS       = 1000;
const unsigned long PAGE_INTERVAL_MS       = 3000;
const unsigned long BALANCE_POLL_MS        = 10000;
const unsigned long REPORT_INTERVAL_MS     = 30000;
const unsigned long REPORT_RETRY_MS        = 15000;
const unsigned long WIFI_RETRY_MS          = 15000;
const unsigned long HTTP_TIMEOUT_MS        = 6000;

const uint8_t FAILS_BEFORE_SENSOR_OFF      = 3;

// Send a consumption report when at least this much energy is
// pending, even if the periodic report timer has not elapsed.
const float REPORT_MIN_WH = 1.0f;

// ============================================================
// PZEM DATA
// ============================================================

struct PzemData {
  float voltage;
  float current;
  float power;
  float energy;       // PZEM cumulative energy in kWh
  float frequency;
  float pf;
};

enum PzemStatus {
  PZEM_OK,
  PZEM_NO_REPLY,
  PZEM_BAD_FRAME,
  PZEM_BAD_CRC
};

PzemData data = {0, 0, 0, 0, 0, 0};
bool valid = false;
uint8_t failCount = 0;

// ============================================================
// LOCAL PAYGO STATE
// ============================================================
//
// serverBalanceKWh = last trusted balance received from backend.
// pendingConsumptionWh = consumption not yet acknowledged by backend.
// available balance = serverBalanceKWh - pendingConsumptionWh/1000.
//
// The pending value survives reboot.
//
// activeReportId / activeReportWh survive reboot too. This is what
// prevents a "server committed but response was lost" condition from
// causing a second deduction.
// ============================================================

Preferences prefs;

double serverBalanceKWh = 0.0;
double pendingConsumptionWh = 0.0;

float lastMeterEnergyWh = 0.0f;
bool meterBaselineReady = false;

String activeReportId = "";
double activeReportWh = 0.0;
unsigned long reportSequence = 0;

bool haveTrustedBalance = false;
bool testBalanceInitialized = false;

unsigned long lastRead = 0;
unsigned long lastPage = 0;
unsigned long lastBalancePoll = 0;
unsigned long lastReportAttempt = 0;
unsigned long lastWiFiRetry = 0;

uint8_t page = 0;
bool relayState = false;

// ============================================================
// UTILITY
// ============================================================

double availableBalanceKWh() {
  double available = serverBalanceKWh - (pendingConsumptionWh / 1000.0);
  if (available < 0.0) available = 0.0;
  return available;
}

void saveState() {
  prefs.putDouble("serverBal", serverBalanceKWh);
  prefs.putDouble("pendingWh", pendingConsumptionWh);
  prefs.putFloat("lastMeter", lastMeterEnergyWh);
  prefs.putBool("meterReady", meterBaselineReady);
  prefs.putString("activeId", activeReportId);
  prefs.putDouble("activeWh", activeReportWh);
  prefs.putULong("reportSeq", reportSequence);
  prefs.putBool("trustedBal", haveTrustedBalance);
  prefs.putBool("testInit", testBalanceInitialized);
}

void loadState() {
  serverBalanceKWh = prefs.getDouble("serverBal", 0.0);
  pendingConsumptionWh = prefs.getDouble("pendingWh", 0.0);
  lastMeterEnergyWh = prefs.getFloat("lastMeter", 0.0f);
  meterBaselineReady = prefs.getBool("meterReady", false);
  activeReportId = prefs.getString("activeId", "");
  activeReportWh = prefs.getDouble("activeWh", 0.0);
  reportSequence = prefs.getULong("reportSeq", 0);
  haveTrustedBalance = prefs.getBool("trustedBal", false);
  testBalanceInitialized = prefs.getBool("testInit", false);

  // Sanity protection against corrupted NVS values.
  if (serverBalanceKWh < 0.0 || serverBalanceKWh > 1000000.0) {
    serverBalanceKWh = 0.0;
    haveTrustedBalance = false;
  }

  if (pendingConsumptionWh < 0.0 || pendingConsumptionWh > 100000000.0) {
    pendingConsumptionWh = 0.0;
  }

  if (activeReportWh < 0.0 || activeReportWh > pendingConsumptionWh + 0.001) {
    activeReportId = "";
    activeReportWh = 0.0;
  }
}

// ============================================================
// RELAY CONTROL
// ============================================================

void stopBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
}

void setRelays(bool on) {
  if (relayState == on) return;

  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);

  if (!on) {
    // The low-energy warning must stop immediately when the load
    // is disconnected.
    stopBuzzer();
  }

  Serial.printf("RELAYS: %s\n", on ? "ON" : "OFF");
}

void enforceEnergyLimit() {
  double available = availableBalanceKWh();

  // Safety rule:
  // - no trusted balance -> OFF
  // - invalid PZEM -> OFF after repeated failures
  // - zero/negative available energy -> OFF
  // - otherwise -> ON
  bool shouldBeOn = haveTrustedBalance && valid && (available > 0.0);

  if (relayState && !shouldBeOn) {
    setRelays(false);

    if (available <= 0.0) {
      Serial.println("PAYGO: balance exhausted. Load disconnected.");
    } else if (!valid) {
      Serial.println("PAYGO: PZEM measurement unavailable. Load disconnected.");
    }
  } else if (!relayState && shouldBeOn) {
    setRelays(true);
  }
}

// ============================================================
// NON-BLOCKING LOW-ENERGY WARNING
// ============================================================
// When available energy is below 1 kWh and the load is connected,
// the buzzer gives a short beep once every second. No delay() is
// used here, so PZEM monitoring, balance accounting, Wi-Fi and
// relay control continue running normally.

const double LOW_ENERGY_LIMIT_KWH = 1.0;
const unsigned long LOW_ENERGY_BEEP_INTERVAL_MS = 1000;
const unsigned long LOW_ENERGY_BEEP_DURATION_MS = 100;

bool lowEnergyBuzzerOn = false;
unsigned long lowEnergyBuzzerStarted = 0;
unsigned long lastLowEnergyBeep = 0;

void serviceLowEnergyWarning() {
  unsigned long now = millis();
  double available = availableBalanceKWh();

  bool warningRequired =
    relayState &&
    haveTrustedBalance &&
    valid &&
    (available > 0.0) &&
    (available < LOW_ENERGY_LIMIT_KWH);

  if (!warningRequired) {
    lowEnergyBuzzerOn = false;
    stopBuzzer();
    return;
  }

  if (lowEnergyBuzzerOn) {
    if (now - lowEnergyBuzzerStarted >= LOW_ENERGY_BEEP_DURATION_MS) {
      lowEnergyBuzzerOn = false;
      stopBuzzer();
    }
    return;
  }

  if (now - lastLowEnergyBeep >= LOW_ENERGY_BEEP_INTERVAL_MS) {
    lastLowEnergyBeep = now;
    lowEnergyBuzzerStarted = now;
    lowEnergyBuzzerOn = true;
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

// ============================================================
// WIFI
// ============================================================

void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not connected. Meter will use local state and retry.");
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();

  if (now - lastWiFiRetry < WIFI_RETRY_MS) return;

  lastWiFiRetry = now;
  Serial.println("Wi-Fi disconnected. Reconnecting...");
  WiFi.reconnect();
}

// ============================================================
// MODBUS CRC16
// ============================================================

uint16_t crc16(const uint8_t* d, uint8_t len) {
  uint16_t crc = 0xFFFF;

  for (uint8_t i = 0; i < len; i++) {
    crc ^= d[i];

    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 1)
             ? (crc >> 1) ^ 0xA001
             : (crc >> 1);
    }
  }

  return crc;
}

// ============================================================
// PZEM READ
// ============================================================

PzemStatus pzemReadOnce(PzemData& out, uint8_t& bytesGot) {
  uint8_t req[8] = {
    PZEM_ADDR,
    0x04,
    0x00,
    0x00,
    0x00,
    0x0A,
    0,
    0
  };

  uint16_t c = crc16(req, 6);
  req[6] = c & 0xFF;
  req[7] = c >> 8;

  while (Serial2.available()) {
    Serial2.read();
  }

  Serial2.write(req, 8);

  uint8_t buf[25];
  uint8_t n = 0;

  unsigned long start = millis();

  while (n < 25 && millis() - start < 500) {
    if (Serial2.available()) {
      buf[n++] = Serial2.read();
    }
  }

  bytesGot = n;

  if (n == 0) return PZEM_NO_REPLY;

  if (n != 25 ||
      buf[1] != 0x04 ||
      buf[2] != 0x14) {
    return PZEM_BAD_FRAME;
  }

  uint16_t rc = crc16(buf, 23);

  if (buf[23] != (rc & 0xFF) ||
      buf[24] != (rc >> 8)) {
    return PZEM_BAD_CRC;
  }

  auto reg = [&](uint8_t i) -> uint32_t {
    return ((uint32_t)buf[3 + 2 * i] << 8) |
           buf[4 + 2 * i];
  };

  out.voltage   = reg(0) / 10.0f * VOLT_CAL;
  out.current   = ((reg(2) << 16) | reg(1)) / 1000.0f;
  out.power     = ((reg(4) << 16) | reg(3)) / 10.0f;
  out.energy    = ((reg(6) << 16) | reg(5)) / 1000.0f;
  out.frequency = reg(7) / 10.0f;
  out.pf        = reg(8) / 100.0f;

  return PZEM_OK;
}

PzemStatus pzemRead(PzemData& out, uint8_t& bytesGot) {
  PzemStatus status = PZEM_NO_REPLY;

  for (uint8_t i = 0; i < PZEM_TRIES; i++) {
    status = pzemReadOnce(out, bytesGot);

    if (status == PZEM_OK) {
      return status;
    }

    delay(100);
  }

  return status;
}

// ============================================================
// ENERGY ACCOUNTING
// ============================================================
//
// PZEM reports cumulative energy. We convert only the increase
// since the last locally accounted reading into pending energy.
//
// The PZEM reset command is intentionally NOT implemented here.
// Resetting the PZEM counter would break the consumption ledger.
// ============================================================

void accountPzemEnergy() {
  float currentMeterEnergyWh = data.energy * 1000.0f;

  if (!meterBaselineReady) {
    // First-ever startup: establish a baseline without charging
    // historical energy that was consumed before this firmware
    // started maintaining its ledger.
    lastMeterEnergyWh = currentMeterEnergyWh;
    meterBaselineReady = true;
    saveState();

    Serial.printf("PZEM baseline established: %.3f Wh\n",
                  currentMeterEnergyWh);
    return;
  }

  // If the PZEM counter moved backwards, assume it was reset or
  // replaced. Do not create a huge negative/positive consumption.
  if (currentMeterEnergyWh + 0.01f < lastMeterEnergyWh) {
    Serial.printf(
      "PZEM energy counter moved backwards %.3f -> %.3f Wh. "
      "Re-baselining.\n",
      lastMeterEnergyWh,
      currentMeterEnergyWh
    );

    lastMeterEnergyWh = currentMeterEnergyWh;
    saveState();
    return;
  }

  float deltaWh = currentMeterEnergyWh - lastMeterEnergyWh;

  // Ignore tiny floating point noise.
  if (deltaWh >= 0.001f) {
    pendingConsumptionWh += deltaWh;
    lastMeterEnergyWh = currentMeterEnergyWh;

    saveState();

    Serial.printf(
      "Energy delta: %.3f Wh | Pending: %.3f Wh | Available: %.6f kWh\n",
      deltaWh,
      pendingConsumptionWh,
      availableBalanceKWh()
    );
  }
}

// ============================================================
// LCD
// ============================================================

uint8_t findLcdAddr() {
  const uint8_t candidates[] = {0x27, 0x3F};

  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);

    if (Wire.endTransmission() == 0) {
      return a;
    }
  }

  return 0x27;
}

void lcdLine(uint8_t row, const String& text) {
  String s = text;

  while (s.length() < LCD_COLS) {
    s += ' ';
  }

  lcd->setCursor(0, row);
  lcd->print(s.substring(0, LCD_COLS));
}

void updateLcd() {
  if (!valid) {
    lcdLine(0, "PZEM no reply");
    lcdLine(1, relayState ? "LOAD: ON" : "LOAD: OFF");
    return;
  }

  switch (page) {
    case 0:
      lcdLine(0, "V:" + String(data.voltage, 1) +
                  " I:" + String(data.current, 2));
      lcdLine(1, "P:" + String(data.power, 0) + "W " +
                  (relayState ? "ON" : "OFF"));
      break;

    case 1:
      lcdLine(0, "PF:" + String(data.pf, 2) +
                  " F:" + String(data.frequency, 1));
      lcdLine(1, "E:" + String(data.energy, 3) + "kWh");
      break;

    case 2:
      lcdLine(0, "Bal:" + String(availableBalanceKWh(), 3) + "kWh");
      lcdLine(1, relayState ? "LOAD: ON" : "LOAD: OFF");
      break;

    case 3:
      lcdLine(0, "Pend:" +
                  String(pendingConsumptionWh / 1000.0, 4));
      lcdLine(1, WiFi.status() == WL_CONNECTED
                    ? "WiFi: OK"
                    : "WiFi: OFF");
      break;
  }
}

// ============================================================
// HTTP HELPERS
// ============================================================

bool prepareHttp(HTTPClient& http,
                 WiFiClientSecure& client,
                 const String& url) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // For a prototype this allows HTTPS without installing the
  // backend's root certificate. For production, replace
  // setInsecure() with the server CA certificate.
  client.setInsecure();

  if (!http.begin(client, url)) {
    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  if (strlen(DEVICE_API_KEY) > 0) {
    http.addHeader("X-Device-Key", DEVICE_API_KEY);
  }

  return true;
}

// ============================================================
// TEST BALANCE FUNCTION
// ============================================================
// This function replaces the API during initial hardware testing.
// It starts at 100 kWh on the first test run. After that, the
// existing NVS state is retained so measured consumption is not
// lost when the ESP32 reboots.
//
// IMPORTANT: This function does NOT subtract energy directly.
// The normal accountPzemEnergy() function still adds measured
// consumption to pendingConsumptionWh, and availableBalanceKWh()
// performs the deduction. That is the same accounting path used
// with the real backend balance.

bool getTestEnergyBalance() {
  if (!testBalanceInitialized) {
    serverBalanceKWh = TEST_INITIAL_BALANCE_KWH;
    pendingConsumptionWh = 0.0;
    activeReportId = "";
    activeReportWh = 0.0;
    haveTrustedBalance = true;
    testBalanceInitialized = true;
    saveState();

    Serial.printf("TEST BALANCE: initialized at %.3f kWh\n",
                  TEST_INITIAL_BALANCE_KWH);
    return true;
  }

  // Keep the persisted simulated purchase balance and pending
  // consumption exactly as they were before a reboot.
  haveTrustedBalance = true;

  Serial.printf(
    "TEST BALANCE: purchased=%.6f kWh | available=%.6f kWh\n",
    serverBalanceKWh,
    availableBalanceKWh()
  );

  return true;
}

// ============================================================
// BALANCE SOURCE SELECTOR
// ============================================================

bool getCurrentEnergyBalance() {
#if USE_TEST_BALANCE
  return getTestEnergyBalance();
#else
  // BACKEND MODE: uncomment the API function below and use this line.
  // return fetchBackendBalance();
  return false;
#endif
}

// ============================================================
// GET CURRENT BACKEND BALANCE
// ============================================================
//
// Existing backend endpoint:
// GET /api/devices/:deviceId
//
// Response contains:
// {
//   "success": true,
//   "deviceId": "...",
//   "balance": 25.5,
//   ...
// }
//
// We deliberately do not replace our local server balance with
// a GET response while pendingConsumptionWh > 0. Otherwise a
// successful backend deduction followed by a lost HTTP response
// could be temporarily subtracted twice locally. The pending
// report is retried with the same reportId and reconciled first.
// ============================================================

/*
// BACKEND API BALANCE FUNCTION
// ------------------------------------------------------------
// Keep this function for backend integration. It is commented
// out during initial testing so no API request is made.

bool fetchBackendBalance() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // Do not overwrite the trusted balance while there is local
  // unacknowledged consumption.
  if (pendingConsumptionWh > 0.00001) {
    return false;
  }

  WiFiClientSecure client;
  HTTPClient http;

  String url = String(API_BASE_URL) +
               "/api/devices/" +
               String(DEVICE_ID);

  if (!prepareHttp(http, client, url)) {
    return false;
  }

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Balance GET failed. HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.printf("Balance JSON error: %s\n", error.c_str());
    return false;
  }

  if (!doc["success"].is<bool>() ||
      !doc["success"].as<bool>()) {
    Serial.println("Balance API returned success=false");
    return false;
  }

  double balance = doc["balance"].as<double>();

  if (isnan(balance) || balance < 0.0) {
    Serial.println("Invalid balance received from backend");
    return false;
  }

  serverBalanceKWh = balance;
  haveTrustedBalance = true;

  saveState();

  Serial.printf(
    "Backend balance: %.6f kWh | Available: %.6f kWh\n",
    serverBalanceKWh,
    availableBalanceKWh()
  );

  return true;
}

*/

// ============================================================
// CREATE / RETRY AN IDEMPOTENT CONSUMPTION REPORT
// ============================================================
//
// New backend endpoint:
// POST /api/devices/:deviceId/consumption
//
// Body:
// {
//   "reportId": "DEVICE-001-123",
//   "consumedWh": 1.234,
//   "meterEnergyWh": 12345.678
// }
//
// The backend must store reportId and make it unique per device.
// A repeated reportId MUST NOT deduct the same energy twice.
// ============================================================

String createReportId() {
  reportSequence++;

  String id = String(DEVICE_ID) +
              "-" +
              String(reportSequence);

  saveState();

  return id;
}

bool submitConsumptionReport() {
  if (!haveTrustedBalance || pendingConsumptionWh <= 0.00001) {
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // Create a report snapshot only once. If the HTTP response is
  // lost, this same report is retried.
  if (activeReportId.length() == 0) {
    activeReportId = createReportId();
    activeReportWh = pendingConsumptionWh;
    saveState();
  }

  if (activeReportWh <= 0.00001) {
    activeReportId = "";
    activeReportWh = 0.0;
    saveState();
    return false;
  }

  WiFiClientSecure client;
  HTTPClient http;

  String url = String(API_BASE_URL) +
               "/api/devices/" +
               String(DEVICE_ID) +
               "/consumption";

  if (!prepareHttp(http, client, url)) {
    return false;
  }

  JsonDocument requestDoc;

  requestDoc["reportId"] = activeReportId;
  requestDoc["consumedWh"] = activeReportWh;
  requestDoc["meterEnergyWh"] = data.energy * 1000.0;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  Serial.printf(
    "Uploading consumption: %.6f kWh, reportId=%s\n",
    activeReportWh / 1000.0,
    activeReportId.c_str()
  );

  int httpCode = http.POST(requestBody);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Consumption POST failed. HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  JsonDocument responseDoc;
  DeserializationError error =
    deserializeJson(responseDoc, response);

  if (error) {
    Serial.printf(
      "Consumption response JSON error: %s\n",
      error.c_str()
    );
    return false;
  }

  if (!responseDoc["success"].is<bool>() ||
      !responseDoc["success"].as<bool>()) {
    Serial.println("Consumption API returned success=false");
    return false;
  }

  double backendBalance =
    responseDoc["balance"].as<double>();

  double appliedKWh =
    responseDoc["appliedKWh"].as<double>();

  if (isnan(backendBalance) ||
      isnan(appliedKWh) ||
      backendBalance < 0.0 ||
      appliedKWh < 0.0) {
    Serial.println("Invalid consumption response values");
    return false;
  }

  // The entire active report has now been durably handled by the
  // backend. Even if appliedKWh is smaller than requested because
  // the balance reached zero, the whole report is consumed from
  // our pending queue. The relay will be forced OFF.
  pendingConsumptionWh -= activeReportWh;

  if (pendingConsumptionWh < 0.00001) {
    pendingConsumptionWh = 0.0;
  }

  serverBalanceKWh = backendBalance;
  haveTrustedBalance = true;

  Serial.printf(
    "Consumption accepted. Applied: %.6f kWh | "
    "Backend balance: %.6f kWh | Pending: %.6f kWh\n",
    appliedKWh,
    serverBalanceKWh,
    pendingConsumptionWh / 1000.0
  );

  activeReportId = "";
  activeReportWh = 0.0;

  saveState();

  return true;
}

// ============================================================
// REPORT SCHEDULER
// ============================================================

void serviceConsumptionReporting() {
#if USE_TEST_BALANCE
  // Backend reporting is intentionally disabled during local testing.
  // pendingConsumptionWh remains the exact same accounting ledger.
  return;
#endif

  if (!haveTrustedBalance) return;
  if (pendingConsumptionWh <= 0.00001) return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  unsigned long retryDelay =
    activeReportId.length() > 0
      ? REPORT_RETRY_MS
      : REPORT_INTERVAL_MS;

  if (now - lastReportAttempt < retryDelay) {
    return;
  }

  // Report immediately if local available balance is zero.
  // Otherwise use the minimum/periodic thresholds.
  double available = availableBalanceKWh();

  bool due =
    activeReportId.length() > 0 ||
    (pendingConsumptionWh >= REPORT_MIN_WH) ||
    (available <= 0.0) ||
    (now - lastReportAttempt >= REPORT_INTERVAL_MS);

  if (!due) return;

  lastReportAttempt = now;
  submitConsumptionReport();
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CONFIRM_PIN, INPUT_PULLUP);

  // Fail-safe startup state.
  setRelays(false);
  digitalWrite(BUZZER_PIN, LOW);

  prefs.begin("paygo", false);
  loadState();

  Serial.println();
  Serial.println("========================================");
  Serial.println(" PAYGO SOLAR ENERGY METER");
  Serial.println("========================================");

  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Saved server balance: %.6f kWh\n",
                serverBalanceKWh);
  Serial.printf("Saved pending: %.6f kWh\n",
                pendingConsumptionWh / 1000.0);
  Serial.printf("Active report: %s\n",
                activeReportId.length()
                  ? activeReportId.c_str()
                  : "none");

  Serial2.begin(
    PZEM_BAUD,
    SERIAL_8N1,
    PZEM_RX_PIN,
    PZEM_TX_PIN
  );

  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL);

  uint8_t addr = findLcdAddr();

  lcd = new LiquidCrystal_I2C(
    addr,
    LCD_COLS,
    LCD_ROWS
  );

  lcd->init();
  lcd->backlight();

  lcdLine(0, "PAYGO Meter");
  lcdLine(1, "Starting...");
  Serial.printf("LCD at 0x%02X\n", addr);

  delay(1500);

  lcd->clear();

  startWiFi();

  // Load any saved balance first. This allows a temporary
  // Internet outage to continue using already-purchased energy.
  enforceEnergyLimit();

  // Load the selected balance source. In test mode this is the
  // simulated 100 kWh balance. In backend mode this calls the API.
  getCurrentEnergyBalance();

  enforceEnergyLimit();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  unsigned long now = millis();

  maintainWiFi();

  // ----------------------------------------------------------
  // PZEM measurement
  // ----------------------------------------------------------

  if (now - lastRead >= READ_INTERVAL_MS) {
    lastRead = now;

    uint8_t got = 0;

    PzemStatus status =
      pzemRead(data, got);

    if (status == PZEM_OK) {
      failCount = 0;
      valid = true;

      Serial.printf(
        "V: %.1f V | I: %.3f A | P: %.1f W | "
        "E: %.3f kWh | f: %.1f Hz | PF: %.2f | "
        "Bal: %.6f kWh | Pending: %.6f kWh\n",
        data.voltage,
        data.current,
        data.power,
        data.energy,
        data.frequency,
        data.pf,
        availableBalanceKWh(),
        pendingConsumptionWh / 1000.0
      );

      accountPzemEnergy();
    } else {
      if (failCount < 255) {
        failCount++;
      }

      if (failCount >= FAILS_BEFORE_SENSOR_OFF) {
        valid = false;
      }

      switch (status) {
        case PZEM_NO_REPLY:
          Serial.println(
            "PZEM: no reply. Check AC, 5V/GND and RX/TX."
          );
          break;

        case PZEM_BAD_FRAME:
          Serial.printf(
            "PZEM: bad frame (%u bytes)\n",
            got
          );
          break;

        case PZEM_BAD_CRC:
          Serial.println(
            "PZEM: CRC mismatch."
          );
          break;

        default:
          break;
      }
    }

    enforceEnergyLimit();
    serviceLowEnergyWarning();
    updateLcd();
  }

  // ----------------------------------------------------------
  // Backend balance synchronization
  // ----------------------------------------------------------

  if (now - lastBalancePoll >= BALANCE_POLL_MS) {
    lastBalancePoll = now;

    // Do not overwrite the local trusted balance while there is
    // unreported energy. The pending report is reconciled first.
    if (pendingConsumptionWh <= 0.00001) {
      getCurrentEnergyBalance();
      enforceEnergyLimit();
      serviceLowEnergyWarning();
      updateLcd();
    }
  }

  // ----------------------------------------------------------
  // Consumption reporting
  // ----------------------------------------------------------

  serviceConsumptionReporting();

  // ----------------------------------------------------------
  // Relay enforcement after every accounting/reporting cycle
  // ----------------------------------------------------------

  enforceEnergyLimit();
  serviceLowEnergyWarning();

  // ----------------------------------------------------------
  // LCD page rotation
  // ----------------------------------------------------------

  if (now - lastPage >= PAGE_INTERVAL_MS) {
    lastPage = now;

    page = (page + 1) % 4;

    updateLcd();
  }
}
