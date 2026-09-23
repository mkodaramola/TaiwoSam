/*
  PAYGO PREPAID ENERGY METER - ESP32 + PZEM-004T (v3.0 Modbus) + 16x2 I2C LCD + 2 relays
  TEST BUILD: balance comes from a simulated source (starts at 100 kWh). API code is kept but commented out.

  Save this text as  Azeez_PayGo/Azeez_PayGo.ino  to compile in the Arduino IDE.

  ---------------------------------------------------------------------------
  MODULES (each can be edited independently)
  ---------------------------------------------------------------------------
    1. BALANCE SOURCE   testSync()  [active]   simulated purchased balance
                        apiSync()   [commented] real backend GET/POST
                        balanceSourceSync()    <- the ONE line that selects which one is used
    2. ENERGY ACCOUNTING accountEnergy() / remainingWh()   (same for test and API)
    3. RELAY CONTROL     relayAllowed() / updateRelays() / setRelays()
    4. BUZZER WARNING    lowEnergyWarningActive() / buzzerTick()   (non-blocking)

  ---------------------------------------------------------------------------
  SWITCHING FROM TEST TO THE REAL API (when the backend is ready)
  ---------------------------------------------------------------------------
    a) Uncomment the three API #includes below.
    b) Uncomment the block  "API BALANCE SOURCE"  (between the /* and  markers).
    c) In balanceSourceSync(): comment out  return testSync();  and uncomment  return apiSync();
    d) Set  BALANCE_SOURCE_NEEDS_WIFI  to true.
    e) Delete/ignore the "TEST BALANCE SOURCE" block.
    f) Fill in API_BASE_URL, METER_ID and DEVICE_TOKEN.
  Everything downstream (consumption, deduction, relays, buzzer, LCD, flash storage)
  is identical for both sources because both feed the same function: applyBalance().

  ---------------------------------------------------------------------------
  HOW THE ACCOUNTING WORKS
  ---------------------------------------------------------------------------
    purchasedWh : total energy bought (owned by the balance source / backend)
    consumedWh  : cumulative energy metered by this device (only ever goes up)
    remaining   = purchasedWh - consumedWh          (integer Wh, no float drift)

  Consumption = change in the PZEM's own energy register since the last accounted
  value; that last value is kept in flash, so energy used across a reboot is not lost.
  The meter reports its CUMULATIVE consumed value (never a "deduct X" delta), so a
  retried API request can never deduct the same energy twice.

  Libraries (Library Manager):
    "LiquidCrystal I2C" by Frank de Brabander
    "ArduinoJson" by Benoit Blanchon (v7.x)   <- only needed once the API block is enabled

  Serial Monitor (115200 baud):
    send 'r'  -> reset the PZEM's energy counter (does NOT change the balance)
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Preferences.h>
// --- needed only when the API block is enabled ---
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>

// ===========================================================================
//  USER CONFIGURATION
// ===========================================================================
#define WIFI_SSID     "Premauda_STARLINK2"
#define WIFI_PASS     "Technologies"

// --- API settings (used only by the commented-out API block) ---
#define API_BASE_URL  "https://your-backend-host"   // https only, no trailing slash
#define METER_ID      "METER-001"                   // must exist in the backend
#define DEVICE_TOKEN  "change-me"                   // per-meter secret (Bearer token)
#define API_BALANCE_PATH  "/api/meters/%s/balance"   // GET  -> purchased/consumed
#define API_READING_PATH  "/api/meters/%s/readings"  // POST -> report consumed
#define KEY_PURCHASED "purchasedKwh"
#define KEY_CONSUMED  "consumedKwh"

// --- Test balance settings (used by testSync) ---
#define TEST_PURCHASED_WH   100000UL   // simulated purchase: 100 kWh
#define TEST_RESET_ON_BOOT  1          // 1 = every reboot restarts at a fresh 100 kWh
                                       // 0 = keep the stored consumption across reboots

// Which balance source needs Wi-Fi to be up before it is called?
const bool BALANCE_SOURCE_NEEDS_WIFI = false;   // test = false, API = true

// ===========================================================================
//  PINS  (from the schematic "Azeez", Sheet_1)
// ===========================================================================
#define PZEM_RX_PIN   16   // ESP32 RX2 <- PZEM TX
#define PZEM_TX_PIN   17   // ESP32 TX2 -> PZEM RX
#define I2C_SDA       21
#define I2C_SCL       22
#define RELAY_PIN     5    // "Relays" net -> R1/R5 -> Q4/Q5 (BC547) -> RL2/RL1
#define BUZZER_PIN    23   // "Buzzer_Alert" -> R9 -> Q2 (2N3904) -> TMB-12A05
#define CONF_BTN_PIN  4    // "conf_btn", active LOW, 10k pull-up to 3V3

#define RELAY_ON_LEVEL   HIGH   // NPN low-side driver: HIGH energises the coil
#define RELAY_OFF_LEVEL  LOW

// ---------- PZEM ----------
#define PZEM_ADDR   0xF8 // general address (single device on the bus)
#define PZEM_BAUD   9600
#define PZEM_TRIES  3    // attempts per read before giving up

// Optional voltage correction. 1.000 = no change.
#define VOLT_CAL    1.000f

// ---------- LCD ----------
#define LCD_COLS 16
#define LCD_ROWS 2
LiquidCrystal_I2C *lcd = nullptr;

// ===========================================================================
//  TIMING / LIMITS
// ===========================================================================
const unsigned long READ_INTERVAL_MS      = 1000;    // PZEM read + accounting
const unsigned long PAGE_INTERVAL_MS      = 3000;    // LCD page rotation
const uint8_t       PAGE_COUNT            = 4;
const uint8_t       FAILS_BEFORE_ERROR    = 3;       // LCD shows PZEM error after this many failed cycles

const unsigned long SYNC_INTERVAL_MS      = 60000;   // balance-source sync while credit remains
const unsigned long LOCKED_POLL_MS        = 10000;   // sync faster when cut off, to catch a top-up
const unsigned long SYNC_BACKOFF_MIN_MS   = 5000;    // retry delay after a failed sync...
const unsigned long SYNC_BACKOFF_MAX_MS   = 300000;  // ...doubling up to this
const unsigned long HTTP_TIMEOUT_MS       = 4000;    // API only; keeps relay control responsive

const unsigned long SAVE_INTERVAL_MS      = 30000;   // flash write throttle for changing counters
const unsigned long WIFI_RETRY_MS         = 10000;
const unsigned long WIFI_BOOT_WAIT_MS     = 10000;
const unsigned long BOOT_SYNC_GATE_MS     = 20000;   // relays wait this long for the first sync;
                                                     // after that, a stored balance may be used offline

const unsigned long PZEM_FAULT_TRIP_MS    = 20000;   // relays ON but PZEM silent this long -> trip
const unsigned long PZEM_STARTUP_GRACE_MS = 10000;   // PZEM may need mains after the relay closes
const unsigned long FAULT_RETRY_MS        = 300000;  // re-try closing relays after a meter fault

// Buzzer low-energy warning
const int32_t       LOW_ENERGY_WH         = 1000;    // warn while remaining < 1 kWh
const unsigned long BUZZER_PERIOD_MS      = 1000;    // one beep every 1 second
const unsigned long BUZZER_ON_MS          = 150;     // length of each beep

// ===========================================================================
//  DATA STRUCTURES
// ===========================================================================
struct PzemData {
  float voltage;    // V
  float current;    // A
  float power;      // W
  float energy;     // kWh (display only)
  float frequency;  // Hz
  float pf;         // 0.00 - 1.00
  uint32_t energyWh; // raw PZEM energy register, Wh (used for accounting)
};

enum PzemStatus { PZEM_OK, PZEM_NO_REPLY, PZEM_BAD_FRAME, PZEM_BAD_CRC };

// Persistent prepaid state (all integer Wh)
#define PZ_UNSET 0xFFFFFFFFUL
struct MeterState {
  uint32_t purchasedWh;   // total purchased, from the balance source
  uint32_t consumedWh;    // cumulative energy metered by this device (monotonic)
  uint32_t ackedWh;       // consumedWh value the balance source has confirmed
  uint32_t lastPzemWh;    // PZEM register value already accounted for (PZ_UNSET = none)
  bool     hasBalance;    // true once a balance has been obtained at least once
};

PzemData   data = {0, 0, 0, 0, 0, 0, 0};
MeterState meter = {0, 0, 0, PZ_UNSET, false};
Preferences prefs;

bool     valid = false;          // PZEM data valid
uint8_t  failCount = 0;
uint8_t  page = 0;
unsigned long lastRead = 0, lastPage = 0;

bool     relayOn = false;
unsigned long relayOnSince = 0;
bool     meterFault = false;
unsigned long faultSince = 0;
unsigned long lastPzemOkMs = 0;

bool     stateDirty = false;
unsigned long lastSave = 0;

bool     bootSynced = false;     // first successful balance-source sync done
unsigned long nextSyncAt = 0;
unsigned long syncBackoffMs = SYNC_BACKOFF_MIN_MS;
unsigned long lastWifiAttempt = 0;
uint32_t bootId = 0;             // used by the API block
uint32_t reportSeq = 0;          // used by the API block

// ===========================================================================
//  SMALL HELPERS
// ===========================================================================
int32_t remainingWh() {
  return (int32_t)meter.purchasedWh - (int32_t)meter.consumedWh;
}

// Wh -> "12.345" (integer maths, no float error)
String kwhStr(int32_t wh) {
  char b[20];
  bool neg = wh < 0;
  uint32_t a = neg ? (uint32_t)(-wh) : (uint32_t)wh;
  snprintf(b, sizeof(b), "%s%lu.%03lu", neg ? "-" : "",
           (unsigned long)(a / 1000), (unsigned long)(a % 1000));
  return String(b);
}

// kWh (double) -> Wh, rejecting NaN/negative
uint32_t kwhToWh(double kwh) {
  if (!(kwh >= 0)) return 0;
  return (uint32_t)lround(kwh * 1000.0);
}

// ===========================================================================
//  MODBUS CRC16  (unchanged)
// ===========================================================================
uint16_t crc16(const uint8_t *d, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= d[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

// ===========================================================================
//  PZEM  (as in Azeez.ino, plus raw energyWh)
// ===========================================================================
// Reads 10 input registers from 0x0000 and decodes them.
// Response frame: addr, 0x04, 0x14, 20 data bytes, CRC lo, CRC hi (25 bytes).
PzemStatus pzemReadOnce(PzemData &out, uint8_t &bytesGot) {
  uint8_t req[8] = {PZEM_ADDR, 0x04, 0x00, 0x00, 0x00, 0x0A, 0, 0};
  uint16_t c = crc16(req, 6);
  req[6] = c & 0xFF;
  req[7] = c >> 8;

  while (Serial2.available()) Serial2.read();   // flush stale bytes
  Serial2.write(req, 8);

  uint8_t buf[25];
  uint8_t n = 0;
  unsigned long start = millis();
  while (n < 25 && millis() - start < 500) {
    if (Serial2.available()) buf[n++] = Serial2.read();
  }
  bytesGot = n;

  if (n == 0) return PZEM_NO_REPLY;
  if (n != 25 || buf[1] != 0x04 || buf[2] != 0x14) return PZEM_BAD_FRAME;

  uint16_t rc = crc16(buf, 23);
  if (buf[23] != (rc & 0xFF) || buf[24] != (rc >> 8)) return PZEM_BAD_CRC;

  // Register i sits at buf[3 + 2*i] (high byte) and buf[4 + 2*i] (low byte)
  auto reg = [&](uint8_t i) -> uint32_t {
    return ((uint32_t)buf[3 + 2 * i] << 8) | buf[4 + 2 * i];
  };

  out.voltage   = reg(0) / 10.0f * VOLT_CAL;
  out.current   = ((reg(2) << 16) | reg(1)) / 1000.0f;
  out.power     = ((reg(4) << 16) | reg(3)) / 10.0f;
  out.energyWh  = (reg(6) << 16) | reg(5);
  out.energy    = out.energyWh / 1000.0f;                // Wh -> kWh
  out.frequency = reg(7) / 10.0f;
  out.pf        = reg(8) / 100.0f;
  return PZEM_OK;
}

// Retries a few times before reporting failure
PzemStatus pzemRead(PzemData &out, uint8_t &bytesGot) {
  PzemStatus s = PZEM_NO_REPLY;
  for (uint8_t i = 0; i < PZEM_TRIES; i++) {
    s = pzemReadOnce(out, bytesGot);
    if (s == PZEM_OK) return s;
    delay(100);
  }
  return s;
}

// Resets the PZEM's own energy counter. The prepaid balance is NOT affected:
// consumedWh is a separate cumulative counter. We re-baseline on the next read.
void pzemResetEnergy() {
  uint8_t req[4] = {PZEM_ADDR, 0x42, 0, 0};
  uint16_t c = crc16(req, 2);
  req[2] = c & 0xFF;
  req[3] = c >> 8;
  while (Serial2.available()) Serial2.read();
  Serial2.write(req, 4);
  delay(300);
  while (Serial2.available()) Serial2.read();

  meter.lastPzemWh = PZ_UNSET;   // next good read becomes the new baseline (no deduction)
  stateDirty = true;
  Serial.println("Energy counter reset command sent (balance unchanged)");
}

// ===========================================================================
//  PERSISTENT STATE (NVS flash)
// ===========================================================================
void loadState() {
  prefs.begin("paygo", true);
  meter.purchasedWh = prefs.getUInt("purch", 0);
  meter.consumedWh  = prefs.getUInt("cons", 0);
  meter.ackedWh     = prefs.getUInt("acked", 0);
  meter.lastPzemWh  = prefs.getUInt("pz", PZ_UNSET);
  meter.hasBalance  = prefs.getBool("hasbal", false);
  prefs.end();
  Serial.printf("Stored state: purchased %s kWh, consumed %s kWh, acked %s kWh\n",
                kwhStr(meter.purchasedWh).c_str(), kwhStr(meter.consumedWh).c_str(),
                kwhStr(meter.ackedWh).c_str());
}

// NVS only rewrites a key when its value actually changed, which limits wear.
void saveState() {
  prefs.begin("paygo", false);
  prefs.putUInt("purch", meter.purchasedWh);
  prefs.putUInt("cons", meter.consumedWh);
  prefs.putUInt("acked", meter.ackedWh);
  prefs.putUInt("pz", meter.lastPzemWh);
  prefs.putBool("hasbal", meter.hasBalance);
  prefs.end();
  stateDirty = false;
  lastSave = millis();
}

void saveTick() {
  if (stateDirty && millis() - lastSave >= SAVE_INTERVAL_MS) saveState();
}

// ===========================================================================
//  MODULE 2: ENERGY ACCOUNTING  (identical for test and API balance sources)
// ===========================================================================
// Adds the PZEM energy that has appeared since the last accounted value.
void accountEnergy(uint32_t pzWh) {
  if (meter.lastPzemWh == PZ_UNSET) {            // first ever read / after reset: baseline only
    meter.lastPzemWh = pzWh;
    stateDirty = true;
    return;
  }
  if (pzWh < meter.lastPzemWh) {                 // PZEM counter was reset/replaced/wrapped
    Serial.println("PZEM energy counter went backwards - re-baselined, nothing deducted");
    meter.lastPzemWh = pzWh;
    stateDirty = true;
    return;
  }
  uint32_t delta = pzWh - meter.lastPzemWh;
  if (delta) {
    meter.consumedWh += delta;
    meter.lastPzemWh = pzWh;
    stateDirty = true;
  }
}

// One measurement cycle (read PZEM, account energy, log)
void meterTick() {
  uint8_t got = 0;
  PzemStatus s = pzemRead(data, got);

  if (s == PZEM_OK) {
    failCount = 0;
    valid = true;
    lastPzemOkMs = millis();
    accountEnergy(data.energyWh);
    Serial.printf("V: %.1f V | I: %.3f A | P: %.1f W | f: %.1f Hz | PF: %.2f | used: %s kWh | balance: %s kWh\n",
                  data.voltage, data.current, data.power, data.frequency, data.pf,
                  kwhStr(meter.consumedWh).c_str(), kwhStr(remainingWh()).c_str());
  } else {
    if (failCount < 255) failCount++;
    if (failCount >= FAILS_BEFORE_ERROR) valid = false;

    switch (s) {
      case PZEM_NO_REPLY:
        Serial.println("PZEM: no reply (0 bytes) - check AC on PZEM, 5V/GND, RX/TX wiring");
        break;
      case PZEM_BAD_FRAME:
        Serial.printf("PZEM: bad frame (%u bytes) - noise or wrong protocol\n", got);
        break;
      case PZEM_BAD_CRC:
        Serial.println("PZEM: CRC mismatch - noise on the serial lines");
        break;
      default: break;
    }
  }
}

// ===========================================================================
//  MODULE 1: BALANCE SOURCE
//  Both sources end by calling applyBalance(); nothing else in the firmware
//  knows or cares which one is active.
// ===========================================================================

// Merges a balance from ANY source into the local state.
//   purchasedWh       : total purchased (the source is the authority on this)
//   sourceConsumedWh  : cumulative consumption the source currently holds
void applyBalance(uint32_t purchasedWh, uint32_t sourceConsumedWh) {
  meter.purchasedWh = purchasedWh;
  if (sourceConsumedWh > meter.consumedWh)   // source ahead (e.g. flash wiped): adopt it,
    meter.consumedWh = sourceConsumedWh;     //   never credit the user back
  meter.ackedWh    = sourceConsumedWh;       // what the source now holds
  meter.hasBalance = true;
  saveState();
}

// ---------------------------------------------------------------------------
//  TEST BALANCE SOURCE  (ACTIVE)  - simulates a backend that sold 100 kWh.
//  Delete this block when the API is enabled.
// ---------------------------------------------------------------------------
bool testSync() {
  static bool initialised = false;
  if (!initialised) {
    initialised = true;
#if TEST_RESET_ON_BOOT
    meter.consumedWh = 0;                    // start a fresh test run
#endif
    Serial.printf("TEST balance source: %s kWh purchased\n", kwhStr(TEST_PURCHASED_WH).c_str());
  }
  // A real backend would return purchased + the consumption we last reported;
  // the simulation simply acknowledges what the meter currently holds.
  applyBalance(TEST_PURCHASED_WH, meter.consumedWh);
  Serial.printf("Sync OK (TEST) -> purchased %s, consumed %s, balance %s kWh\n",
                kwhStr(meter.purchasedWh).c_str(), kwhStr(meter.consumedWh).c_str(),
                kwhStr(remainingWh()).c_str());
  return true;
}

// ---------------------------------------------------------------------------
//  API BALANCE SOURCE  (COMMENTED OUT until the backend is ready)
//  bootSynced == false -> GET the balance.  Afterwards -> POST the cumulative reading.
// ---------------------------------------------------------------------------
/*
// Returns HTTP status (<= 0 = transport error). Response body in `resp`.
int httpRequest(bool isPost, const char *path, const String &body, String &resp) {
  WiFiClientSecure client;
  client.setInsecure();   // PROTOTYPE: no certificate check. For production pin the
                          // server's root CA with client.setCACert(...) instead.
  HTTPClient http;
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);

  String url = String(API_BASE_URL) + path;
  if (!http.begin(client, url)) return -1;
  http.addHeader("Authorization", String("Bearer ") + DEVICE_TOKEN);
  http.addHeader("Content-Type", "application/json");

  int code = isPost ? http.POST(body) : http.GET();
  if (code > 0) resp = http.getString();
  http.end();
  return code;
}

// Accepts a JSON number or a numeric string (SQL DECIMAL often arrives as text)
bool readNumber(JsonVariant v, double &out) {
  if (v.is<double>()) { out = v.as<double>(); return true; }
  if (v.is<const char *>()) {
    const char *s = v.as<const char *>();
    char *end = nullptr;
    out = strtod(s, &end);
    return end != s;
  }
  return false;
}

// Parses {purchasedKwh, consumedKwh} (top level or inside "data") and feeds applyBalance().
// Returns false (and changes nothing) if anything is wrong.
bool applyServerReply(const String &resp) {
  JsonDocument doc;
  if (deserializeJson(doc, resp)) return false;

  JsonVariant root = doc["data"].is<JsonObject>() ? doc["data"].as<JsonVariant>()
                                                   : doc.as<JsonVariant>();
  double p, c;
  JsonVariant vp = root[KEY_PURCHASED];
  JsonVariant vc = root[KEY_CONSUMED];
  if (!readNumber(vp, p) || !readNumber(vc, c) || p < 0 || c < 0) return false;

  applyBalance(kwhToWh(p), kwhToWh(c));
  return true;
}

bool apiSync() {
  if (WiFi.status() != WL_CONNECTED) return false;

  char path[96];
  String resp, body;
  int code;

  if (!bootSynced) {
    snprintf(path, sizeof(path), API_BALANCE_PATH, METER_ID);
    code = httpRequest(false, path, "", resp);
  } else {
    snprintf(path, sizeof(path), API_READING_PATH, METER_ID);

    char consumed[20];
    snprintf(consumed, sizeof(consumed), "%lu.%03lu",
             (unsigned long)(meter.consumedWh / 1000), (unsigned long)(meter.consumedWh % 1000));

    JsonDocument doc;
    doc["deviceId"]    = METER_ID;
    doc["bootId"]      = bootId;
    doc["seq"]         = ++reportSeq;
    doc["consumedKwh"] = serialized(consumed);        // cumulative, absolute, monotonic
    doc["relayOn"]     = relayOn;
    if (valid) {
      doc["voltage"]     = data.voltage;
      doc["current"]     = data.current;
      doc["power"]       = data.power;
      doc["frequency"]   = data.frequency;
      doc["powerFactor"] = data.pf;
    }
    serializeJson(doc, body);
    code = httpRequest(true, path, body, resp);
  }

  bool ok = (code >= 200 && code < 300) && applyServerReply(resp);
  if (ok) {
    Serial.printf("Sync OK (%s) -> purchased %s, consumed %s, balance %s kWh\n",
                  bootSynced ? "POST" : "GET",
                  kwhStr(meter.purchasedWh).c_str(), kwhStr(meter.consumedWh).c_str(),
                  kwhStr(remainingWh()).c_str());
  } else {
    Serial.printf("Sync FAILED (HTTP %d)%s\n", code,
                  (code == 401 || code == 403) ? " - check DEVICE_TOKEN" :
                  (code == 404) ? " - check METER_ID / endpoint path" : "");
  }
  return ok;
}
*/

// ---------------------------------------------------------------------------
//  SOURCE SELECTOR - the only line to change when swapping test <-> API
// ---------------------------------------------------------------------------
bool balanceSourceSync() {
  return testSync();          // TEST source (active)
  // return apiSync();        // REAL API source (enable the API block above first)
}

// Schedules and runs balance-source syncs with exponential back-off on failure.
void syncTick() {
  if ((long)(millis() - nextSyncAt) < 0) return;                          // not due yet
  if (BALANCE_SOURCE_NEEDS_WIFI && WiFi.status() != WL_CONNECTED) return; // stays due; fires when Wi-Fi is back

  if (balanceSourceSync()) {
    bootSynced = true;
    syncBackoffMs = SYNC_BACKOFF_MIN_MS;
    nextSyncAt = millis() + (remainingWh() <= 0 ? LOCKED_POLL_MS : SYNC_INTERVAL_MS);
  } else {
    nextSyncAt = millis() + syncBackoffMs;                                // exponential back-off
    syncBackoffMs = min(syncBackoffMs * 2, SYNC_BACKOFF_MAX_MS);
  }
}

// ===========================================================================
//  MODULE 4: BUZZER LOW-ENERGY WARNING  (non-blocking)
//  Beeps once per second while the load is connected and remaining < 1 kWh.
//  Pure millis() timing - no delay() - so metering and relay control never wait on it.
// ===========================================================================
bool buzzerIsOn = false;
bool buzzerWasActive = false;
unsigned long buzzerCycleStart = 0;

// Condition for the warning - change this one function to change the rule.
bool lowEnergyWarningActive() {
  return relayOn && remainingWh() < LOW_ENERGY_WH;
}

void buzzerWrite(bool on) {
  if (on != buzzerIsOn) {
    buzzerIsOn = on;
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
  }
}

// Silence immediately (used when the relays open)
void buzzerStop() {
  buzzerWasActive = false;
  buzzerWrite(false);
}

// Call every loop pass
void buzzerTick() {
  if (!lowEnergyWarningActive()) {          // balance >= 1 kWh, or load disconnected
    buzzerStop();
    return;
  }
  unsigned long now = millis();
  if (!buzzerWasActive) {                   // warning just started: begin a fresh cycle
    buzzerWasActive = true;
    buzzerCycleStart = now;
  }
  unsigned long phase = (now - buzzerCycleStart) % BUZZER_PERIOD_MS;
  buzzerWrite(phase < BUZZER_ON_MS);        // ON for the first 150 ms of every second
}

// ===========================================================================
//  MODULE 3: RELAY CONTROL
// ===========================================================================
void updateLcd();   // forward declaration

void setRelays(bool on) {
  if (on == relayOn) return;
  digitalWrite(RELAY_PIN, on ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
  relayOn = on;
  if (on) relayOnSince = millis();
  else    buzzerStop();                     // load disconnected -> silence the warning at once
  Serial.printf("Relays %s (balance %s kWh)\n", on ? "ON" : "OFF", kwhStr(remainingWh()).c_str());
  saveState();                              // persist the latest counters immediately
  updateLcd();
}

// Should the load be powered right now? Change this to change the cut-off rule.
bool relayAllowed() {
  if (!meter.hasBalance) return false;                              // never had a balance
  if (!bootSynced && millis() < BOOT_SYNC_GATE_MS) return false;    // wait for first sync
  if (meterFault) return false;                                     // PZEM lost while load on
  return remainingWh() > 0;                                         // the prepaid rule
}

void updateRelays() {
  unsigned long now = millis();

  // Anti-bypass: if the PZEM stops answering while the load is on, energy is not
  // being measured -> cut the load. Clears when the PZEM answers again, or retries
  // after FAULT_RETRY_MS (needed if the PZEM only gets mains through the relays).
  bool pzemSilent = (now - lastPzemOkMs) > PZEM_FAULT_TRIP_MS;
  if (!meterFault && relayOn && pzemSilent && (now - relayOnSince) > PZEM_STARTUP_GRACE_MS) {
    meterFault = true;
    faultSince = now;
    Serial.println("METER FAULT: PZEM silent while load ON - relays will open");
  }
  if (meterFault && (!pzemSilent || now - faultSince >= FAULT_RETRY_MS)) {
    meterFault = false;
    lastPzemOkMs = now;
    Serial.println("Meter fault cleared/retrying");
  }

  setRelays(relayAllowed());
}

// ===========================================================================
//  WI-FI  (non-blocking reconnect; metering and relays never wait on it)
// ===========================================================================
void wifiBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastWifiAttempt = millis();
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttempt >= WIFI_RETRY_MS) {
    lastWifiAttempt = millis();
    Serial.println("Wi-Fi down - reconnecting");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

// ===========================================================================
//  LCD HELPERS
// ===========================================================================
uint8_t findLcdAddr() {
  const uint8_t candidates[] = {0x27, 0x3F};
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) return a;
  }
  return 0x27;   // fall back to the most common address
}

// Print a line padded to full width so no old characters are left behind
void lcdLine(uint8_t row, const String &text) {
  String s = text;
  while (s.length() < LCD_COLS) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, LCD_COLS));
}

void updateLcd() {
  // Load is off: say why
  if (!relayOn) {
    if (!meter.hasBalance)            { lcdLine(0, "Connecting...");  lcdLine(1, "Getting balance"); }
    else if (meterFault)              { lcdLine(0, "Meter fault");    lcdLine(1, "Check PZEM/wire"); }
    else if (remainingWh() <= 0)      { lcdLine(0, "No credit");      lcdLine(1, "Recharge online"); }
    else                              { lcdLine(0, "Load OFF");       lcdLine(1, "Please wait..."); }
    return;
  }

  if (!valid) {
    lcdLine(0, "PZEM no reply");
    lcdLine(1, "Check wiring/AC");
    return;
  }

  switch (page) {
    case 0:
      lcdLine(0, "Bal:" + kwhStr(remainingWh()) + " kWh");
      lcdLine(1, String("Load:ON  WiFi:") + (WiFi.status() == WL_CONNECTED ? "OK" : "--"));
      break;
    case 1:
      lcdLine(0, "Volt: " + String(data.voltage, 1) + " V");
      lcdLine(1, "Curr: " + String(data.current, 3) + " A");
      break;
    case 2:
      lcdLine(0, "Pwr: " + String(data.power, 1) + " W");
      lcdLine(1, "PF:" + String(data.pf, 2) + " " + String(data.frequency, 1) + "Hz");
      break;
    case 3:
      lcdLine(0, "Used:" + kwhStr(meter.consumedWh) + "kWh");
      lcdLine(1, String("Sync:") + (meter.consumedWh == meter.ackedWh ? "OK" : "PEND"));
      break;
  }
}

// ===========================================================================
//  INPUTS
// ===========================================================================
void handleSerial() {
  if (Serial.available() && Serial.read() == 'r') pzemResetEnergy();
}

// Confirm button: request an immediate balance sync (useful right after a top-up)
void handleButton() {
  static bool lastLevel = HIGH;
  static unsigned long lastChange = 0;
  bool level = digitalRead(CONF_BTN_PIN);
  if (level != lastLevel && millis() - lastChange > 50) {
    lastChange = millis();
    lastLevel = level;
    if (level == LOW) {
      nextSyncAt = millis();
      syncBackoffMs = SYNC_BACKOFF_MIN_MS;
      Serial.println("Button: sync requested");
    }
  }
}

// ===========================================================================
//  SETUP / LOOP
// ===========================================================================
void setup() {
  // Load OFF before anything else; only switched on once the balance is verified
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(CONF_BTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial2.begin(PZEM_BAUD, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL);
  uint8_t addr = findLcdAddr();
  lcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
  lcd->init();
  lcd->backlight();
  lcdLine(0, "PAYGO Meter");
  lcdLine(1, "Starting...");
  Serial.printf("LCD at 0x%02X\n", addr);

  loadState();
  bootId = esp_random();
  lastPzemOkMs = millis();

  wifiBegin();
  lcdLine(1, "Connecting WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_BOOT_WAIT_MS) delay(250);
  Serial.println(WiFi.status() == WL_CONNECTED ? "Wi-Fi connected" : "Wi-Fi not connected yet");

  lcd->clear();
}

void loop() {
  unsigned long now = millis();

  handleSerial();
  handleButton();
  maintainWifi();

  if (now - lastRead >= READ_INTERVAL_MS) {
    lastRead = now;
    meterTick();                 // measure + deduct
    updateLcd();
  }

  updateRelays();                // evaluated every pass, independent of Wi-Fi/API state
  buzzerTick();                  // non-blocking low-energy beeper
  syncTick();                    // balance source (test now, API later)
  saveTick();

  if (now - lastPage >= PAGE_INTERVAL_MS) {
    lastPage = now;
    page = (page + 1) % PAGE_COUNT;
    updateLcd();
  }
}