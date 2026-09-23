#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <RTClib.h>
#include <HTTPClient.h>

// ===================== Wi-Fi / Firebase =====================

#define WIFI_SSID       "Premauda_STARLINK2"
#define WIFI_PASSWORD   "Technologies"

#define FIREBASE_HOST   "https://smartsystems-962b2-default-rtdb.firebaseio.com"

// ===================== Pins =====================

#define PZEM_RX_PIN     16
#define PZEM_TX_PIN     17

#define I2C_SDA         21
#define I2C_SCL         22

#define SD_CS_PIN       5
#define SD_MOSI_PIN     23
#define SD_MISO_PIN     19
#define SD_SCK_PIN      18

#define RELAY_PIN       4
#define BUZZER_PIN      13

#define RELAY_ON_LEVEL  HIGH
#define RELAY_OFF_LEVEL LOW

// ===================== PZEM =====================

#define PZEM_ADDR       0xF8
#define PZEM_BAUD       9600
#define PZEM_TRIES      3
#define VOLT_CAL        1.000f

// ===================== LCD =====================

#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C *lcd = nullptr;

// ===================== RTC =====================

RTC_DS3231 rtc;

// ===================== Preferences =====================

Preferences preferences;


// ===================== Firebase =====================



// ===================== Keypad =====================

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] =
{
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};

byte rowPins[ROWS] =
{
  34,
  35,
  32,
  33
};

byte colPins[COLS] =
{
  25,
  26,
  27,
  14
};

Keypad keypad =
  Keypad(
    makeKeymap(keys),
    rowPins,
    colPins,
    ROWS,
    COLS
  );

// ===================== PZEM data =====================

struct PzemData
{
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float pf;
};

PzemData data = {0, 0, 0, 0, 0, 0};

enum PzemStatus
{
  PZEM_OK,
  PZEM_NO_REPLY,
  PZEM_BAD_FRAME,
  PZEM_BAD_CRC
};

bool pzemValid = false;
uint8_t pzemFailCount = 0;

const uint8_t FAILS_BEFORE_ERROR = 3;

// ===================== Energy management =====================

float monthlyBudget = 1000.0f;

float dailyBudget = 0.0f;

float monthlyStartEnergy = -1.0f;
float dailyStartEnergy = -1.0f;

float monthlyEnergyUsed = 0.0f;
float dailyEnergyUsed = 0.0f;

float monthlyEnergyRemaining = 0.0f;
float dailyEnergyRemaining = 0.0f;

float dailyPercentageUsed = 0.0f;

float lastPzemEnergy = -1.0f;

bool energyInitialized = false;

bool loadCutOff = false;
bool manualLoadOff = false;

uint8_t warningLevel = 0;

uint16_t currentYear = 0;
uint8_t currentMonth = 0;
uint8_t currentDay = 0;

// ===================== Firebase state =====================

bool firebaseAvailable = false;

unsigned long lastFirebaseUpload = 0;
unsigned long lastFirebaseRead = 0;
unsigned long lastFirebaseReconnect = 0;

const unsigned long FIREBASE_UPLOAD_INTERVAL = 5000;
const unsigned long FIREBASE_READ_INTERVAL = 3000;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;

// ===================== General timers =====================

unsigned long lastPzemRead = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastLog = 0;
unsigned long lastBudgetCheck = 0;
unsigned long lastPageChange = 0;
unsigned long lastPreferencesSave = 0;

const unsigned long PZEM_INTERVAL = 1000;
const unsigned long LCD_INTERVAL = 1000;
const unsigned long SD_LOG_INTERVAL = 10000;
const unsigned long BUDGET_INTERVAL = 1000;
const unsigned long PAGE_INTERVAL = 4000;
const unsigned long PREFERENCES_SAVE_INTERVAL = 60000;

uint8_t lcdPage = 0;

// ============================================================
//                         Wi-Fi
// ============================================================

void maintainWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  unsigned long now = millis();

  if (
    now - lastFirebaseReconnect >=
    WIFI_RECONNECT_INTERVAL
  )
  {
    lastFirebaseReconnect = now;

    Serial.println("Wi-Fi disconnected. Reconnecting...");

    WiFi.disconnect();
    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );
  }
}

// ============================================================
//                         CRC16
// ============================================================

uint16_t crc16(
  const uint8_t *d,
  uint8_t len
)
{
  uint16_t crc = 0xFFFF;

  for (uint8_t i = 0; i < len; i++)
  {
    crc ^= d[i];

    for (uint8_t b = 0; b < 8; b++)
    {
      crc =
        (crc & 1)
        ? (crc >> 1) ^ 0xA001
        : crc >> 1;
    }
  }

  return crc;
}

// ============================================================
//                       PZEM READ
// ============================================================

PzemStatus pzemReadOnce(
  PzemData &out,
  uint8_t &bytesGot
)
{
  uint8_t req[8] =
  {
    PZEM_ADDR,
    0x04,
    0x00,
    0x00,
    0x00,
    0x0A,
    0,
    0
  };

  uint16_t c =
    crc16(req, 6);

  req[6] = c & 0xFF;
  req[7] = c >> 8;

  while (Serial2.available())
  {
    Serial2.read();
  }

  Serial2.write(
    req,
    8
  );

  uint8_t buf[25];
  uint8_t n = 0;

  unsigned long start =
    millis();

  while (
    n < 25 &&
    millis() - start < 500
  )
  {
    if (Serial2.available())
    {
      buf[n++] =
        Serial2.read();
    }
  }

  bytesGot = n;

  if (n == 0)
  {
    return PZEM_NO_REPLY;
  }

  if (
    n != 25 ||
    buf[1] != 0x04 ||
    buf[2] != 0x14
  )
  {
    return PZEM_BAD_FRAME;
  }

  uint16_t rc =
    crc16(buf, 23);

  if (
    buf[23] != (rc & 0xFF) ||
    buf[24] != (rc >> 8)
  )
  {
    return PZEM_BAD_CRC;
  }

  auto reg =
    [&](uint8_t i) -> uint32_t
  {
    return
      ((uint32_t)buf[3 + 2 * i] << 8) |
      buf[4 + 2 * i];
  };

  out.voltage =
    reg(0) / 10.0f * VOLT_CAL;

  out.current =
    ((reg(2) << 16) | reg(1))
    / 1000.0f;

  out.power =
    ((reg(4) << 16) | reg(3))
    / 10.0f;

  out.energy =
    ((reg(6) << 16) | reg(5))
    / 1000.0f;

  out.frequency =
    reg(7) / 10.0f;

  out.pf =
    reg(8) / 100.0f;

  return PZEM_OK;
}

PzemStatus pzemRead(
  PzemData &out,
  uint8_t &bytesGot
)
{
  PzemStatus st =
    PZEM_NO_REPLY;

  for (
    uint8_t i = 0;
    i < PZEM_TRIES;
    i++
  )
  {
    st =
      pzemReadOnce(
        out,
        bytesGot
      );

    if (st == PZEM_OK)
    {
      return st;
    }

    delay(100);
  }

  return st;
}

// ============================================================
//                    PZEM ENERGY RESET
// ============================================================

void pzemResetEnergy()
{
  uint8_t req[4] =
  {
    PZEM_ADDR,
    0x42,
    0,
    0
  };

  uint16_t c =
    crc16(req, 2);

  req[2] = c & 0xFF;
  req[3] = c >> 8;

  while (Serial2.available())
  {
    Serial2.read();
  }

  Serial2.write(
    req,
    4
  );

  delay(300);

  while (Serial2.available())
  {
    Serial2.read();
  }

  if (pzemValid)
  {
    monthlyStartEnergy =
      data.energy;

    dailyStartEnergy =
      data.energy;

    lastPzemEnergy =
      data.energy;

    monthlyEnergyUsed = 0;
    dailyEnergyUsed = 0;

    warningLevel = 0;

    loadCutOff = false;
    manualLoadOff = false;

    setRelay(true);

    saveEnergyState();
  }

  Serial.println(
    "PZEM energy counter reset."
  );
}

// ============================================================
//                         RTC
// ============================================================

uint8_t daysInMonth(
  uint16_t year,
  uint8_t month
)
{
  if (month == 2)
  {
    bool leap =
      (year % 4 == 0 &&
       year % 100 != 0) ||
      (year % 400 == 0);

    return leap ? 29 : 28;
  }

  if (
    month == 4 ||
    month == 6 ||
    month == 9 ||
    month == 11
  )
  {
    return 30;
  }

  return 31;
}

void calculateDailyBudget()
{
  DateTime now =
    rtc.now();

  uint8_t days =
    daysInMonth(
      now.year(),
      now.month()
    );

  dailyBudget =
    monthlyBudget /
    (float)days;
}

// ============================================================
//                     PREFERENCES
// ============================================================

void saveEnergyState()
{
  preferences.begin(
    "energy",
    false
  );

  preferences.putFloat(
    "monthlyBudget",
    monthlyBudget
  );

  preferences.putFloat(
    "monthlyStart",
    monthlyStartEnergy
  );

  preferences.putFloat(
    "dailyStart",
    dailyStartEnergy
  );

  preferences.putFloat(
    "lastPzem",
    lastPzemEnergy
  );

  preferences.putUShort(
    "year",
    currentYear
  );

  preferences.putUChar(
    "month",
    currentMonth
  );

  preferences.putUChar(
    "day",
    currentDay
  );

  preferences.end();
}

void loadEnergyState()
{
  preferences.begin(
    "energy",
    true
  );

  monthlyBudget =
    preferences.getFloat(
      "monthlyBudget",
      1000.0f
    );

  monthlyStartEnergy =
    preferences.getFloat(
      "monthlyStart",
      -1.0f
    );

  dailyStartEnergy =
    preferences.getFloat(
      "dailyStart",
      -1.0f
    );

  lastPzemEnergy =
    preferences.getFloat(
      "lastPzem",
      -1.0f
    );

  currentYear =
    preferences.getUShort(
      "year",
      0
    );

  currentMonth =
    preferences.getUChar(
      "month",
      0
    );

  currentDay =
    preferences.getUChar(
      "day",
      0
    );

  preferences.end();
}

// ============================================================
//                   RELAY CONTROL
// ============================================================

void setRelay(
  bool on
)
{
  if (on)
  {
    digitalWrite(
      RELAY_PIN,
      RELAY_ON_LEVEL
    );
  }
  else
  {
    digitalWrite(
      RELAY_PIN,
      RELAY_OFF_LEVEL
    );
  }
}

bool isLoadOn()
{
  return
    digitalRead(RELAY_PIN) ==
    RELAY_ON_LEVEL;
}

// ============================================================
//                     ENERGY ACCOUNTING
// ============================================================

void initializeEnergyState()
{
  if (!pzemValid)
  {
    return;
  }

  DateTime now =
    rtc.now();

  bool newState =
    currentYear == 0 ||
    currentMonth == 0 ||
    currentDay == 0;

  bool newMonth =
    !newState &&
    (
      currentYear != now.year() ||
      currentMonth != now.month()
    );

  bool newDay =
    !newState &&
    currentDay != now.day();

  if (newState || newMonth)
  {
    currentYear =
      now.year();

    currentMonth =
      now.month();

    currentDay =
      now.day();

    monthlyStartEnergy =
      data.energy;

    dailyStartEnergy =
      data.energy;

    lastPzemEnergy =
      data.energy;

    monthlyEnergyUsed = 0;
    dailyEnergyUsed = 0;

    warningLevel = 0;
    loadCutOff = false;

    setRelay(true);

    calculateDailyBudget();

    saveEnergyState();

    energyInitialized = true;

    return;
  }

  if (newDay)
  {
    currentDay =
      now.day();

    dailyStartEnergy =
      data.energy;

    dailyEnergyUsed = 0;

    warningLevel = 0;
    loadCutOff = false;

    setRelay(true);

    calculateDailyBudget();

    saveEnergyState();

    energyInitialized = true;

    return;
  }

  if (
    monthlyStartEnergy < 0 ||
    dailyStartEnergy < 0
  )
  {
    monthlyStartEnergy =
      data.energy;

    dailyStartEnergy =
      data.energy;

    lastPzemEnergy =
      data.energy;

    monthlyEnergyUsed = 0;
    dailyEnergyUsed = 0;

    calculateDailyBudget();

    saveEnergyState();
  }

  energyInitialized = true;
}

void calculateEnergyUsage()
{
  if (
    !pzemValid ||
    !energyInitialized
  )
  {
    return;
  }

  if (
    lastPzemEnergy >= 0 &&
    data.energy < lastPzemEnergy
  )
  {
    lastPzemEnergy =
      data.energy;

    monthlyStartEnergy =
      data.energy -
      monthlyEnergyUsed;

    dailyStartEnergy =
      data.energy -
      dailyEnergyUsed;

    if (monthlyStartEnergy < 0)
      monthlyStartEnergy = 0;

    if (dailyStartEnergy < 0)
      dailyStartEnergy = 0;

    saveEnergyState();

    return;
  }

  if (
    monthlyStartEnergy >= 0 &&
    data.energy >= monthlyStartEnergy
  )
  {
    monthlyEnergyUsed =
      data.energy -
      monthlyStartEnergy;
  }

  if (
    dailyStartEnergy >= 0 &&
    data.energy >= dailyStartEnergy
  )
  {
    dailyEnergyUsed =
      data.energy -
      dailyStartEnergy;
  }

  lastPzemEnergy =
    data.energy;

  monthlyEnergyRemaining =
    monthlyBudget -
    monthlyEnergyUsed;

  dailyEnergyRemaining =
    dailyBudget -
    dailyEnergyUsed;

  if (monthlyEnergyRemaining < 0)
    monthlyEnergyRemaining = 0;

  if (dailyEnergyRemaining < 0)
    dailyEnergyRemaining = 0;

  if (dailyBudget > 0)
  {
    dailyPercentageUsed =
      (
        dailyEnergyUsed /
        dailyBudget
      ) * 100.0f;
  }
  else
  {
    dailyPercentageUsed = 0;
  }
}

// ============================================================
//                       WARNINGS
// ============================================================

void beep(
  uint16_t duration
)
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  delay(duration);

  digitalWrite(
    BUZZER_PIN,
    LOW
  );
}

void processEnergyLimit()
{
  if (!pzemValid)
  {
    return;
  }

  if (
    dailyPercentageUsed >= 70 &&
    warningLevel < 1
  )
  {
    beep(200);
    warningLevel = 1;

    Serial.println(
      "70% daily energy limit reached."
    );
  }

  if (
    dailyPercentageUsed >= 80 &&
    warningLevel < 2
  )
  {
    beep(250);

    delay(100);

    beep(250);

    warningLevel = 2;

    Serial.println(
      "80% daily energy limit reached."
    );
  }

  if (
    dailyEnergyUsed >= dailyBudget &&
    !loadCutOff
  )
  {
    loadCutOff = true;

    setRelay(false);

    beep(1000);

    Serial.println(
      "Daily energy limit reached. Load OFF."
    );
  }
}

// ============================================================
//                     DATE MANAGEMENT
// ============================================================

void checkDateChange()
{
  if (!pzemValid)
  {
    return;
  }

  DateTime now =
    rtc.now();

  if (
    now.year() != currentYear ||
    now.month() != currentMonth
  )
  {
    currentYear =
      now.year();

    currentMonth =
      now.month();

    currentDay =
      now.day();

    monthlyStartEnergy =
      data.energy;

    dailyStartEnergy =
      data.energy;

    lastPzemEnergy =
      data.energy;

    monthlyEnergyUsed = 0;
    dailyEnergyUsed = 0;

    warningLevel = 0;
    loadCutOff = false;

    setRelay(true);

    calculateDailyBudget();

    saveEnergyState();

    return;
  }

  if (
    now.day() != currentDay
  )
  {
    currentDay =
      now.day();

    dailyStartEnergy =
      data.energy;

    lastPzemEnergy =
      data.energy;

    dailyEnergyUsed = 0;

    warningLevel = 0;
    loadCutOff = false;

    setRelay(true);

    calculateDailyBudget();

    saveEnergyState();
  }
}

// ============================================================
//                         LCD
// ============================================================

uint8_t findLcdAddr()
{
  const uint8_t addresses[] =
  {
    0x27,
    0x3F
  };

  for (
    uint8_t i = 0;
    i < 2;
    i++
  )
  {
    Wire.beginTransmission(
      addresses[i]
    );

    if (
      Wire.endTransmission() == 0
    )
    {
      return addresses[i];
    }
  }

  return 0x27;
}

void lcdLine(
  uint8_t row,
  const String &text
)
{
  String s = text;

  while (
    s.length() < LCD_COLS
  )
  {
    s += ' ';
  }

  lcd->setCursor(
    0,
    row
  );

  lcd->print(
    s.substring(
      0,
      LCD_COLS
    )
  );
}

void updateLCD()
{
  if (!lcd)
    return;

  if (!pzemValid)
  {
    lcdLine(
      0,
      "NO POWER"
    );

    lcdLine(
      1,
      "Check PZEM"
    );

    return;
  }

  switch (lcdPage)
  {
    case 0:

      lcdLine(
        0,
        "V:" +
        String(data.voltage, 1) +
        " I:" +
        String(data.current, 2)
      );

      lcdLine(
        1,
        "P:" +
        String(data.power, 0) +
        "W"
      );

      break;

    case 1:

      lcdLine(
        0,
        "E:" +
        String(data.energy, 3) +
        "kWh"
      );

      lcdLine(
        1,
        "PF:" +
        String(data.pf, 2) +
        " F:" +
        String(data.frequency, 1)
      );

      break;

    case 2:

      lcdLine(
        0,
        "Used:" +
        String(dailyEnergyUsed, 2)
      );

      lcdLine(
        1,
        "Limit:" +
        String(dailyBudget, 2)
      );

      break;

    case 3:

      lcdLine(
        0,
        "Used:" +
        String(dailyPercentageUsed, 1) +
        "%"
      );

      lcdLine(
        1,
        "Rem:" +
        String(dailyEnergyRemaining, 2)
      );

      break;

    case 4:

      if (loadCutOff)
      {
        lcdLine(
          0,
          "LOAD: OFF"
        );

        lcdLine(
          1,
          "ENERGY LIMIT"
        );
      }
      else if (manualLoadOff)
      {
        lcdLine(
          0,
          "LOAD: OFF"
        );

        lcdLine(
          1,
          "MANUAL"
        );
      }
      else
      {
        lcdLine(
          0,
          "LOAD: ON"
        );

        lcdLine(
          1,
          firebaseAvailable
            ? "IoT ONLINE"
            : "IoT OFFLINE"
        );
      }

      break;

    case 5:

      lcdLine(
        0,
        "Budget:" +
        String(monthlyBudget, 1) +
        "kWh"
      );

      lcdLine(
        1,
        "Daily:" +
        String(dailyBudget, 2) +
        "kWh"
      );

      break;
  }
}

// ============================================================
//                     SD CARD
// ============================================================

bool initializeSD()
{
  SPI.begin(
    SD_SCK_PIN,
    SD_MISO_PIN,
    SD_MOSI_PIN,
    SD_CS_PIN
  );

  if (!SD.begin(SD_CS_PIN))
  {
    Serial.println(
      "SD card unavailable."
    );

    return false;
  }

  if (!SD.exists("/energy.csv"))
  {
    File file =
      SD.open(
        "/energy.csv",
        FILE_WRITE
      );

    if (file)
    {
      file.println(
        "Date,Time,Voltage,Current,Power,Energy,Frequency,PF,EnergyUsed,PercentageUsed,MonthlyBudget,LoadOut"
      );

      file.close();
    }
  }

  return true;
}

void logEnergyToSD()
{
  if (!pzemValid)
    return;

  File file =
    SD.open(
      "/energy.csv",
      FILE_APPEND
    );

  if (!file)
    return;

  DateTime now =
    rtc.now();

  char dateBuffer[11];
  char timeBuffer[9];

  snprintf(
    dateBuffer,
    sizeof(dateBuffer),
    "%04d-%02d-%02d",
    now.year(),
    now.month(),
    now.day()
  );

  snprintf(
    timeBuffer,
    sizeof(timeBuffer),
    "%02d:%02d:%02d",
    now.hour(),
    now.minute(),
    now.second()
  );

  file.print(dateBuffer);
  file.print(",");

  file.print(timeBuffer);
  file.print(",");

  file.print(data.voltage, 2);
  file.print(",");

  file.print(data.current, 3);
  file.print(",");

  file.print(data.power, 2);
  file.print(",");

  file.print(data.energy, 3);
  file.print(",");

  file.print(data.frequency, 2);
  file.print(",");

  file.print(data.pf, 2);
  file.print(",");

  file.print(dailyEnergyUsed, 3);
  file.print(",");

  file.print(dailyPercentageUsed, 2);
  file.print(",");

  file.print(monthlyBudget, 3);
  file.print(",");

  file.println(
    isLoadOn()
      ? "ON"
      : "OFF"
  );

  file.close();
}

// ============================================================
//                     FIREBASE UPLOAD
// ============================================================

void uploadFirebaseData()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    firebaseAvailable = false;
    return;
  }

  HTTPClient http;

  String url =
    String(FIREBASE_HOST) +
    "/loadmang.json";

  http.begin(url);

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  String jsonData =
    "{";

  jsonData += "\"voltage\":" +
    String(data.voltage, 5);

  jsonData += ",\"current\":" +
    String(data.current, 5);

  jsonData += ",\"power\":" +
    String(data.power, 5);

  jsonData += ",\"energy\":" +
    String(data.energy, 5);

  jsonData += ",\"frequency\":" +
    String(data.frequency, 5);

  jsonData += ",\"pf\":" +
    String(data.pf, 5);

  jsonData += ",\"energyUsed\":" +
    String(dailyEnergyUsed, 5);

  jsonData += ",\"percentageUsed\":" +
    String(dailyPercentageUsed, 5);

  jsonData += ",\"loadout\":" +
    String(isLoadOn() ? "true" : "false");

  jsonData += "}";

  // NOTE: changed from PUT to PATCH. PUT replaces the ENTIRE
  // /loadmang node, which was wiping out /loadmang/monthlybudget
  // (and any other child not listed above) every 5 seconds.
  // PATCH only updates the fields included in jsonData and leaves
  // everything else, like monthlybudget, untouched.
  int httpResponseCode =
    http.PATCH(jsonData);

  if (
    httpResponseCode >= 200 &&
    httpResponseCode < 300
  )
  {
    firebaseAvailable = true;
  }
  else
  {
    firebaseAvailable = false;

    Serial.print(
      "Firebase upload failed: "
    );

    Serial.println(
      httpResponseCode > 0
        ? http.getString()
        : http.errorToString(
            httpResponseCode
          ).c_str()
    );
  }

  http.end();
}

// ============================================================
//                 FIREBASE REMOTE CONTROL
// ============================================================

void readFirebaseControls()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    firebaseAvailable = false;
    return;
  }

  float remoteBudget = 0;

  // ----------------------------------------------------------
  // Read monthly budget
  // ----------------------------------------------------------

  {
    HTTPClient http;

    String url =
      String(FIREBASE_HOST) +
      "/loadmang/monthlybudget/budget.json";

    http.begin(url);

    int httpResponseCode =
      http.GET();

    if (
      httpResponseCode >= 200 &&
      httpResponseCode < 300
    )
    {
      String response =
        http.getString();

      response.trim();

      if (
        response.length() > 0 &&
        response != "null"
      )
      {
        remoteBudget =
          response.toFloat();

        if (
          remoteBudget > 0 &&
          fabs(
            remoteBudget -
            monthlyBudget
          ) > 0.001f
        )
        {
          monthlyBudget =
            remoteBudget;

          calculateDailyBudget();

          saveEnergyState();

          Serial.print(
            "Firebase monthly budget: "
          );

          Serial.println(
            monthlyBudget
          );
        }

        firebaseAvailable = true;
      }
    }
    else
    {
      Serial.print(
        "Budget read failed: "
      );

      Serial.println(
        httpResponseCode > 0
          ? http.getString()
          : http.errorToString(
              httpResponseCode
            ).c_str()
      );

      firebaseAvailable = false;
    }

    http.end();
  }

  // ----------------------------------------------------------
  // Read load state
  // ----------------------------------------------------------

  bool remoteLoadState;

  {
    HTTPClient http;

    String url =
      String(FIREBASE_HOST) +
      "/loadmang/loadout.json";

    http.begin(url);

    int httpResponseCode =
      http.GET();

    if (
      httpResponseCode >= 200 &&
      httpResponseCode < 300
    )
    {
      String response =
        http.getString();

      response.trim();

      if (response == "true")
      {
        remoteLoadState = true;

        firebaseAvailable = true;

        manualLoadOff = false;

        if (
          !loadCutOff &&
          dailyEnergyUsed < dailyBudget
        )
        {
          setRelay(true);
        }
      }
      else if (response == "false")
      {
        remoteLoadState = false;

        firebaseAvailable = true;

        manualLoadOff = true;

        setRelay(false);
      }
      else
      {
        Serial.print(
          "Invalid Firebase load state: "
        );

        Serial.println(
          response
        );

        firebaseAvailable = false;
      }
    }
    else
    {
      Serial.print(
        "Load state read failed: "
      );

      Serial.println(
        httpResponseCode > 0
          ? http.getString()
          : http.errorToString(
              httpResponseCode
            ).c_str()
      );

      firebaseAvailable = false;
    }

    http.end();
  }
}

// ============================================================
//                    KEYPAD BUDGET
// ============================================================


// ============================================================

void enterMonthlyBudget()
{
  lcd->clear();

  lcdLine(
    0,
    "Monthly kWh:"
  );

  lcdLine(
    1,
    "A=. #=Save"
  );

  String input = "";

  while (true)
  {
    char key =
      keypad.getKey();

    if (!key)
    {
      delay(20);
      continue;
    }

    if (
      key >= '0' &&
      key <= '9'
    )
    {
      if (input.length() < 8)
      {
        input += key;
      }

      lcdLine(
        1,
        input
      );
    }

    else if (key == 'A')
    {
      if (
        input.indexOf('.') == -1
      )
      {
        input += '.';
      }

      lcdLine(
        1,
        input
      );
    }

    else if (key == '*')
    {
      input = "";

      lcdLine(
        1,
        ""
      );
    }

    else if (key == '#')
    {
      float value =
        input.toFloat();

      if (value <= 0)
      {
        lcdLine(
          0,
          "Invalid budget"
        );

        delay(1200);

        lcd->clear();

        lcdLine(
          0,
          "Monthly kWh:"
        );

        input = "";

        continue;
      }

      monthlyBudget =
        value;

      calculateDailyBudget();

      saveEnergyState();

      if (
        WiFi.status() ==
        WL_CONNECTED
      )
      {
        HTTPClient http;

        String url =
          String(FIREBASE_HOST) +
          "/loadmang/monthlybudget/budget.json";

        http.begin(url);

        http.addHeader(
          "Content-Type",
          "application/json"
        );

        int httpResponseCode =
          http.PUT(
            String(monthlyBudget, 5)
          );

        if (
          httpResponseCode >= 200 &&
          httpResponseCode < 300
        )
        {
          firebaseAvailable = true;
        }
        else
        {
          firebaseAvailable = false;

          Serial.print(
            "Firebase budget update failed: "
          );

          Serial.println(
            httpResponseCode > 0
              ? http.getString()
              : http.errorToString(
                  httpResponseCode
                ).c_str()
          );
        }

        http.end();
      }

      lcd->clear();

      lcdLine(
        0,
        "Budget Saved"
      );

      lcdLine(
        1,
        String(
          monthlyBudget,
          1
        ) +
        " kWh"
      );

      delay(1500);

      lcd->clear();

      return;
    }
  }
}

// ============================================================
//                      KEYPAD
// ============================================================

void processKeypad()
{
  char key =
    keypad.getKey();

  if (!key)
    return;

  if (key == 'A')
  {
    enterMonthlyBudget();
    return;
  }

  if (key == 'B')
  {
    lcd->clear();

    lcdLine(
      0,
      "Budget:" +
      String(
        monthlyBudget,
        1
      )
    );

    lcdLine(
      1,
      "Daily:" +
      String(
        dailyBudget,
        2
      )
    );

    delay(2000);

    lcd->clear();

    return;
  }

  if (key == 'C')
  {
    lcd->clear();

    lcdLine(
      0,
      "Used:" +
      String(
        dailyEnergyUsed,
        2
      )
    );

    lcdLine(
      1,
      String(
        dailyPercentageUsed,
        1
      ) +
      "% used"
    );

    delay(2000);

    lcd->clear();

    return;
  }

  if (key == 'D')
  {
    if (
      !loadCutOff &&
      dailyEnergyUsed < dailyBudget
    )
    {
      manualLoadOff = false;

      setRelay(true);

      if (
        WiFi.status() ==
        WL_CONNECTED
      )
      {
        HTTPClient http;

        String url =
          String(FIREBASE_HOST) +
          "/loadmang/loadout.json";

        http.begin(url);

        http.addHeader(
          "Content-Type",
          "application/json"
        );

        int httpResponseCode =
          http.PUT("true");

        if (
          httpResponseCode >= 200 &&
          httpResponseCode < 300
        )
        {
          firebaseAvailable = true;
        }
        else
        {
          firebaseAvailable = false;

          Serial.print(
            "Firebase load state update failed: "
          );

          Serial.println(
            httpResponseCode > 0
              ? http.getString()
              : http.errorToString(
                  httpResponseCode
                ).c_str()
          );
        }

        http.end();
      }

      lcd->clear();

      lcdLine(
        0,
        "LOAD ON"
      );

      lcdLine(
        1,
        "Manual Restore"
      );

      delay(1200);

      lcd->clear();
    }
    else
    {
      lcd->clear();

      lcdLine(
        0,
        "Cannot Restore"
      );

      lcdLine(
        1,
        "Energy Limit"
      );

      delay(1200);

      lcd->clear();
    }

    return;
  }

  if (key == '#')
  {
    lcdPage++;

    if (lcdPage >= 6)
      lcdPage = 0;

    updateLCD();
  }
}

// ============================================================
//                    SERIAL STATUS
// ============================================================

void printStatus()
{
  Serial.println();
  Serial.println(
    "========== LOAD MANAGEMENT =========="
  );

  Serial.printf(
    "WiFi: %s\n",
    WiFi.status() == WL_CONNECTED
      ? "CONNECTED"
      : "DISCONNECTED"
  );

  Serial.printf(
    "Firebase: %s\n",
    firebaseAvailable
      ? "CONNECTED"
      : "OFFLINE"
  );

  Serial.printf(
    "Voltage: %.2f V\n",
    data.voltage
  );

  Serial.printf(
    "Current: %.3f A\n",
    data.current
  );

  Serial.printf(
    "Power: %.2f W\n",
    data.power
  );

  Serial.printf(
    "PZEM Energy: %.3f kWh\n",
    data.energy
  );

  Serial.printf(
    "PF: %.2f\n",
    data.pf
  );

  Serial.printf(
    "Frequency: %.2f Hz\n",
    data.frequency
  );

  Serial.printf(
    "Monthly Budget: %.3f kWh\n",
    monthlyBudget
  );

  Serial.printf(
    "Daily Budget: %.3f kWh\n",
    dailyBudget
  );

  Serial.printf(
    "Energy Used: %.3f kWh\n",
    dailyEnergyUsed
  );

  Serial.printf(
    "Percentage Used: %.2f %%\n",
    dailyPercentageUsed
  );

  Serial.printf(
    "Load: %s\n",
    isLoadOn()
      ? "ON"
      : "OFF"
  );

  Serial.println(
    "====================================="
  );
}

// ============================================================
//                         SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  pinMode(
    RELAY_PIN,
    OUTPUT
  );

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  setRelay(true);

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  Serial2.begin(
    PZEM_BAUD,
    SERIAL_8N1,
    PZEM_RX_PIN,
    PZEM_TX_PIN
  );

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );

  uint8_t lcdAddr =
    findLcdAddr();

  lcd =
    new LiquidCrystal_I2C(
      lcdAddr,
      LCD_COLS,
      LCD_ROWS
    );

  lcd->init();
  lcd->backlight();

  lcdLine(
    0,
    "Load Management"
  );

  lcdLine(
    1,
    "Starting..."
  );

  if (!rtc.begin())
  {
    lcdLine(
      0,
      "RTC ERROR"
    );

    Serial.println("Couldn't find RTC");
    Serial.flush();

    lcdLine(
      1,
      "Check RTC"
    );

    delay(1500);
  }

   if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


  loadEnergyState();

  initializeSD();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.println(
    "Connecting Wi-Fi..."
  );

  unsigned long wifiStart =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - wifiStart < 10000
  )
  {
    delay(100);
  }

  if (
    WiFi.status() ==
    WL_CONNECTED
  )
  {
    Serial.print(
      "Wi-Fi IP: "
    );

    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    Serial.println(
      "Wi-Fi unavailable. Continuing offline."
    );
  }



  uint8_t got = 0;

  PzemStatus st =
    pzemRead(
      data,
      got
    );

  if (st == PZEM_OK)
  {
    pzemValid = true;

    pzemFailCount = 0;

    initializeEnergyState();

    calculateDailyBudget();

    calculateEnergyUsage();
  }

  if (
    WiFi.status() ==
    WL_CONNECTED
  )
  {
    readFirebaseControls();

    uploadFirebaseData();
  }

  lcd->clear();

  if (pzemValid)
  {
    lcdLine(
      0,
      "System Ready"
    );

    lcdLine(
      1,
      isLoadOn()
        ? "LOAD ON"
        : "LOAD OFF"
    );
  }
  else
  {
    lcdLine(
      0,
      "PZEM ERROR"
    );

    lcdLine(
      1,
      "Check Wiring"
    );
  }

  delay(1200);

  lcd->clear();

  Serial.println(
    "System ready."
  );

  Serial.println(
    "A = Monthly budget"
  );

  Serial.println(
    "B = Budget"
  );

  Serial.println(
    "C = Consumption"
  );

  Serial.println(
    "D = Restore load"
  );

  Serial.println(
    "# = LCD page"
  );

  Serial.println(
    "r = Reset PZEM energy"
  );

  Serial.println(
    "s = System status"
  );
}

// ============================================================
//                         LOOP
// ============================================================

void loop()
{
  unsigned long now =
    millis();

  maintainWiFi();

  if (Serial.available())
  {
    char command =
      Serial.read();

    if (command == 'r')
    {
      pzemResetEnergy();
    }

    if (command == 's')
    {
      printStatus();
    }
  }

  if (
    now - lastPzemRead >=
    PZEM_INTERVAL
  )
  {
    lastPzemRead = now;

    uint8_t got = 0;

    PzemStatus st =
      pzemRead(
        data,
        got
      );

    if (st == PZEM_OK)
    {
      pzemValid = true;
      pzemFailCount = 0;

      if (!energyInitialized)
      {
        initializeEnergyState();
      }

      Serial.printf(
        "V: %.1f V | "
        "I: %.3f A | "
        "P: %.1f W | "
        "E: %.3f kWh | "
        "F: %.1f Hz | "
        "PF: %.2f\n",

        data.voltage,
        data.current,
        data.power,
        data.energy,
        data.frequency,
        data.pf
      );
    }
    else
    {
      if (
        pzemFailCount < 255
      )
      {
        pzemFailCount++;
      }

      if (
        pzemFailCount >=
        FAILS_BEFORE_ERROR
      )
      {
        pzemValid = false;
      }
    }
  }

  if (
    now - lastBudgetCheck >=
    BUDGET_INTERVAL
  )
  {
    lastBudgetCheck = now;

    checkDateChange();

    calculateDailyBudget();

    calculateEnergyUsage();

    processEnergyLimit();
  }

  if (
    now - lastPreferencesSave >=
    PREFERENCES_SAVE_INTERVAL
  )
  {
    lastPreferencesSave = now;

    if (pzemValid)
    {
      saveEnergyState();
    }
  }

  if (
    now - lastFirebaseRead >=
    FIREBASE_READ_INTERVAL
  )
  {
    lastFirebaseRead = now;

    readFirebaseControls();
  }

  if (
    now - lastFirebaseUpload >=
    FIREBASE_UPLOAD_INTERVAL
  )
  {
    lastFirebaseUpload = now;

    uploadFirebaseData();
  }

  if (
    now - lastLog >=
    SD_LOG_INTERVAL
  )
  {
    lastLog = now;

    logEnergyToSD();
  }

  processKeypad();

  if (
    now - lastLcdUpdate >=
    LCD_INTERVAL
  )
  {
    lastLcdUpdate = now;

    updateLCD();
  }

  if (
    now - lastPageChange >=
    PAGE_INTERVAL
  )
  {
    lastPageChange = now;

    lcdPage++;

    if (lcdPage >= 6)
      lcdPage = 0;

    updateLCD();
  }

  delay(5);
}