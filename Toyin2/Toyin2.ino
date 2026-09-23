/*
  ================================================================
      IoT-BASED POWER MONITORING & AUTOMATIC LOAD MANAGEMENT
  ================================================================

  Controller:
      ESP32 DevKit V1 30-Pin

  Peripherals:
      - PZEM-004T v3.0
      - 16x2 I2C LCD
      - DS3231 RTC
      - 4x4 Matrix Keypad
      - SD Card Module
      - Buzzer
      - Relay Load Control

  Main Functions:
      1. Measure electrical parameters using PZEM
      2. Calculate energy consumption
      3. Set monthly energy budget
      4. Calculate daily energy allowance
      5. Monitor daily/monthly consumption
      6. Warn user at 70% and 80%
      7. Disconnect load at 100%
      8. Log energy data to SD card
      9. Display information on LCD
     10. Store settings in ESP32 Preferences

  Required Libraries:

      LiquidCrystal_I2C
      Keypad
      RTClib
      SD
      SPI

  ================================================================
*/


#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <RTClib.h>


// ================================================================
//                         PIN DEFINITIONS
// ================================================================

// ---------- PZEM ----------
#define PZEM_RX_PIN       16
#define PZEM_TX_PIN       17

#define PZEM_ADDR         0xF8
#define PZEM_BAUD         9600
#define PZEM_TRIES        3

#define VOLT_CAL          1.000f


// ---------- I2C ----------
#define I2C_SDA           21
#define I2C_SCL           22


// ---------- SD CARD ----------
#define SD_CS_PIN         5
#define SD_MOSI_PIN       23
#define SD_MISO_PIN       19
#define SD_SCK_PIN        18


// ---------- RELAY ----------
#define RELAY_PIN         4

/*
   The schematic uses an NPN transistor driver.

   Change this to HIGH if your relay driver turns ON with HIGH.

   If your hardware is active LOW, change to LOW.
*/
#define RELAY_ON_LEVEL    HIGH
#define RELAY_OFF_LEVEL   LOW


// ---------- BUZZER ----------
#define BUZZER_PIN        13


// ================================================================
//                         KEYPAD
// ================================================================

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
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


Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);


// ================================================================
//                         LCD
// ================================================================

#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C *lcd = nullptr;


// ================================================================
//                         RTC
// ================================================================

RTC_DS3231 rtc;


// ================================================================
//                         PREFERENCES
// ================================================================

Preferences preferences;


// ================================================================
//                         PZEM DATA
// ================================================================

struct PzemData
{
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float pf;
};


PzemData data =
{
  0,
  0,
  0,
  0,
  0,
  0
};


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


// ================================================================
//                  ENERGY MANAGEMENT VARIABLES
// ================================================================

/*
   Monthly energy budget entered by the user.

   Example:

       1000 kWh/month
*/
float monthlyBudget = 1000.0;


// Energy reading when the current monthly cycle started.

float monthlyStartEnergy = 0.0;


// Current consumption within the monthly cycle.

float monthlyEnergyUsed = 0.0;


// Remaining monthly energy.

float monthlyEnergyRemaining = 0.0;


// Daily energy allowance.

float dailyBudget = 0.0;


// Energy reading when the current day started.

float dailyStartEnergy = 0.0;


// Energy consumed today.

float dailyEnergyUsed = 0.0;


// Remaining energy for today.

float dailyEnergyRemaining = 0.0;


// Current warning level.

uint8_t warningLevel = 0;


// Whether load has been disconnected.

bool loadCutOff = false;


// Current date used for detecting a new day.

uint16_t currentDay = 0;

uint8_t currentMonth = 0;

uint16_t currentYear = 0;


// ================================================================
//                         TIMERS
// ================================================================

unsigned long lastPzemRead = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastLog = 0;
unsigned long lastBudgetCheck = 0;

const unsigned long PZEM_INTERVAL = 1000;
const unsigned long LCD_INTERVAL = 1000;
const unsigned long LOG_INTERVAL = 10000;
const unsigned long BUDGET_INTERVAL = 1000;


// ================================================================
//                         LCD PAGES
// ================================================================

uint8_t lcdPage = 0;

unsigned long lastPageChange = 0;

const unsigned long PAGE_INTERVAL = 4000;


// ================================================================
//                         LCD ADDRESS
// ================================================================

uint8_t findLcdAddr()
{
  const uint8_t candidates[] =
  {
    0x27,
    0x3F
  };

  for (uint8_t i = 0; i < 2; i++)
  {
    Wire.beginTransmission(candidates[i]);

    if (Wire.endTransmission() == 0)
    {
      return candidates[i];
    }
  }

  return 0x27;
}


// ================================================================
//                         LCD LINE
// ================================================================

void lcdLine(
  uint8_t row,
  const String &text
)
{
  String s = text;

  while (s.length() < LCD_COLS)
  {
    s += ' ';
  }

  lcd->setCursor(0, row);

  lcd->print(
    s.substring(0, LCD_COLS)
  );
}


// ================================================================
//                         MODBUS CRC
// ================================================================

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
      if (crc & 1)
      {
        crc =
          (crc >> 1) ^
          0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }

  return crc;
}


// ================================================================
//                         PZEM READ
// ================================================================

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


  req[6] =
    c & 0xFF;

  req[7] =
    c >> 8;


  while (Serial2.available())
  {
    Serial2.read();
  }


  Serial2.write(req, 8);


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


// ================================================================
//                         PZEM RETRIES
// ================================================================

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


// ================================================================
//                     RESET PZEM ENERGY
// ================================================================

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


  req[2] =
    c & 0xFF;

  req[3] =
    c >> 8;


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


  Serial.println(
    "PZEM energy reset command sent"
  );
}


// ================================================================
//                  DAYS IN CURRENT MONTH
// ================================================================

uint8_t daysInMonth(
  uint16_t year,
  uint8_t month
)
{
  if (
    month == 2
  )
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


// ================================================================
//                   CALCULATE DAILY BUDGET
// ================================================================

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


// ================================================================
//                   SAVE SETTINGS
// ================================================================

void saveSettings()
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


  preferences.putUShort(
    "year",
    currentYear
  );


  preferences.putUChar(
    "month",
    currentMonth
  );


  preferences.putUShort(
    "day",
    currentDay
  );


  preferences.end();
}


// ================================================================
//                   LOAD SETTINGS
// ================================================================

void loadSettings()
{
  preferences.begin(
    "energy",
    true
  );


  monthlyBudget =
    preferences.getFloat(
      "monthlyBudget",
      1000.0
    );


  monthlyStartEnergy =
    preferences.getFloat(
      "monthlyStart",
      -1.0
    );


  dailyStartEnergy =
    preferences.getFloat(
      "dailyStart",
      -1.0
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
    preferences.getUShort(
      "day",
      0
    );


  preferences.end();
}


// ================================================================
//                   SAVE NEW MONTHLY BUDGET
// ================================================================

void setMonthlyBudget(
  float newBudget
)
{
  if (newBudget <= 0)
  {
    return;
  }


  monthlyBudget =
    newBudget;


  monthlyStartEnergy =
    data.energy;


  dailyStartEnergy =
    data.energy;


  DateTime now =
    rtc.now();


  currentYear =
    now.year();

  currentMonth =
    now.month();

  currentDay =
    now.day();


  calculateDailyBudget();


  monthlyEnergyUsed =
    0;


  dailyEnergyUsed =
    0;


  monthlyEnergyRemaining =
    monthlyBudget;


  dailyEnergyRemaining =
    dailyBudget;


  warningLevel =
    0;


  loadCutOff =
    false;


  digitalWrite(
    RELAY_PIN,
    RELAY_ON_LEVEL
  );


  saveSettings();


  Serial.println(
    "New monthly energy budget saved."
  );
}


// ================================================================
//                    CHECK NEW DAY / MONTH
// ================================================================

void checkDateChange()
{
  DateTime now =
    rtc.now();


  // ------------------------------------------------
  // New month
  // ------------------------------------------------

  if (
    now.year() != currentYear ||
    now.month() != currentMonth
  )
  {
    Serial.println(
      "New month detected."
    );


    currentYear =
      now.year();

    currentMonth =
      now.month();

    currentDay =
      now.day();


    if (pzemValid)
    {
      monthlyStartEnergy =
        data.energy;

      dailyStartEnergy =
        data.energy;
    }


    warningLevel =
      0;


    loadCutOff =
      false;


    digitalWrite(
      RELAY_PIN,
      RELAY_ON_LEVEL
    );


    calculateDailyBudget();


    saveSettings();


    return;
  }


  // ------------------------------------------------
  // New day
  // ------------------------------------------------

  if (
    now.day() != currentDay
  )
  {
    Serial.println(
      "New day detected."
    );


    currentDay =
      now.day();


    if (pzemValid)
    {
      dailyStartEnergy =
        data.energy;
    }


    warningLevel =
      0;


    loadCutOff =
      false;


    digitalWrite(
      RELAY_PIN,
      RELAY_ON_LEVEL
    );


    calculateDailyBudget();


    saveSettings();
  }
}


// ================================================================
//                     CALCULATE ENERGY USE
// ================================================================

void calculateEnergyUsage()
{
  if (!pzemValid)
  {
    return;
  }


  // Prevent negative values caused by PZEM reset
  if (
    data.energy >=
    monthlyStartEnergy
  )
  {
    monthlyEnergyUsed =
      data.energy -
      monthlyStartEnergy;
  }
  else
  {
    monthlyStartEnergy =
      data.energy;

    monthlyEnergyUsed =
      0;
  }


  if (
    data.energy >=
    dailyStartEnergy
  )
  {
    dailyEnergyUsed =
      data.energy -
      dailyStartEnergy;
  }
  else
  {
    dailyStartEnergy =
      data.energy;

    dailyEnergyUsed =
      0;
  }


  monthlyEnergyRemaining =
    monthlyBudget -
    monthlyEnergyUsed;


  dailyEnergyRemaining =
    dailyBudget -
    dailyEnergyUsed;


  if (
    monthlyEnergyRemaining < 0
  )
  {
    monthlyEnergyRemaining =
      0;
  }


  if (
    dailyEnergyRemaining < 0
  )
  {
    dailyEnergyRemaining =
      0;
  }
}


// ================================================================
//                         BUZZER
// ================================================================

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


// ================================================================
//                     WARNING HANDLER
// ================================================================

void processWarnings()
{
  if (!pzemValid)
  {
    return;
  }


  float dailyPercentage =
    0;


  if (dailyBudget > 0)
  {
    dailyPercentage =
      (
        dailyEnergyUsed /
        dailyBudget
      ) * 100.0f;
  }


  // ------------------------------------------------
  // 70% warning
  // ------------------------------------------------

  if (
    dailyPercentage >= 70 &&
    warningLevel < 1
  )
  {
    Serial.println(
      "WARNING: 70% daily energy budget reached."
    );


    beep(200);


    warningLevel =
      1;
  }


  // ------------------------------------------------
  // 80% warning
  // ------------------------------------------------

  if (
    dailyPercentage >= 80 &&
    warningLevel < 2
  )
  {
    Serial.println(
      "WARNING: 80% daily energy budget reached."
    );


    beep(300);

    delay(100);

    beep(300);


    warningLevel =
      2;
  }


  // ------------------------------------------------
  // 100% cutoff
  // ------------------------------------------------

  if (
    dailyEnergyUsed >=
    dailyBudget
  )
  {
    if (!loadCutOff)
    {
      Serial.println(
        "DAILY ENERGY LIMIT REACHED."
      );


      digitalWrite(
        RELAY_PIN,
        RELAY_OFF_LEVEL
      );


      loadCutOff =
        true;


      warningLevel =
        3;


      beep(1000);
    }
  }
}


// ================================================================
//                     UPDATE LCD
// ================================================================

void updateLCD()
{
  if (!lcd)
  {
    return;
  }


  if (!pzemValid)
  {
    lcdLine(
      0,
      "PZEM ERROR"
    );


    lcdLine(
      1,
      "Check wiring"
    );


    return;
  }


  switch (lcdPage)
  {
    // ------------------------------------------------
    // Page 0
    // Voltage / Current
    // ------------------------------------------------

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


    // ------------------------------------------------
    // Page 1
    // Energy / Power Factor
    // ------------------------------------------------

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


    // ------------------------------------------------
    // Page 2
    // Daily energy
    // ------------------------------------------------

    case 2:

      lcdLine(
        0,
        "Today:" +
        String(dailyEnergyUsed, 2)
      );


      lcdLine(
        1,
        "Limit:" +
        String(dailyBudget, 2)
      );

      break;


    // ------------------------------------------------
    // Page 3
    // Remaining energy
    // ------------------------------------------------

    case 3:

      lcdLine(
        0,
        "Day Rem:" +
        String(dailyEnergyRemaining, 2)
      );


      lcdLine(
        1,
        "Mon Rem:" +
        String(monthlyEnergyRemaining, 1)
      );

      break;


    // ------------------------------------------------
    // Page 4
    // Load state
    // ------------------------------------------------

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
      else
      {
        lcdLine(
          0,
          "LOAD: ON"
        );


        lcdLine(
          1,
          "System Normal"
        );
      }

      break;
  }
}


// ================================================================
//                     SD CARD INITIALIZATION
// ================================================================

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
      "SD card initialization failed."
    );

    return false;
  }


  Serial.println(
    "SD card initialized."
  );


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
        "Date,Time,Voltage,Current,Power,Energy_kWh,Frequency,PF,DailyUsed_kWh,DailyBudget_kWh,MonthlyUsed_kWh,MonthlyBudget_kWh,Load"
      );


      file.close();
    }
  }


  return true;
}


// ================================================================
//                         SD LOGGING
// ================================================================

void logEnergyToSD()
{
  if (!pzemValid)
  {
    return;
  }


  File file =
    SD.open(
      "/energy.csv",
      FILE_APPEND
    );


  if (!file)
  {
    Serial.println(
      "Unable to open energy.csv"
    );

    return;
  }


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

  file.print(dailyBudget, 3);
  file.print(",");

  file.print(monthlyEnergyUsed, 3);
  file.print(",");

  file.print(monthlyBudget, 3);
  file.print(",");

  file.println(
    loadCutOff ? "OFF" : "ON"
  );


  file.close();
}


// ================================================================
//                    KEYPAD BUDGET ENTRY
// ================================================================

void enterMonthlyBudget()
{
  lcd->clear();


  lcdLine(
    0,
    "Monthly kWh:"
  );


  lcdLine(
    1,
    "0#=Save *=Clear"
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


    // --------------------------------------------
    // Numeric entry
    // --------------------------------------------

    if (
      key >= '0' &&
      key <= '9'
    )
    {
      if (input.length() < 6)
      {
        input += key;
      }


      lcdLine(
        1,
        input
      );
    }


    // --------------------------------------------
    // Decimal point
    //
    // Key A is used for decimal point.
    // --------------------------------------------

    else if (key == 'A')
    {
      if (
        input.indexOf('.') == -1 &&
        input.length() < 6
      )
      {
        input += '.';
      }


      lcdLine(
        1,
        input
      );
    }


    // --------------------------------------------
    // Clear
    // --------------------------------------------

    else if (key == '*')
    {
      input = "";

      lcdLine(
        1,
        ""
      );
    }


    // --------------------------------------------
    // Save
    // --------------------------------------------

    else if (key == '#')
    {
      if (input.length() == 0)
      {
        continue;
      }


      float value =
        input.toFloat();


      if (value <= 0)
      {
        lcdLine(
          0,
          "Invalid budget"
        );


        delay(1500);


        lcd->clear();


        lcdLine(
          0,
          "Monthly kWh:"
        );


        lcdLine(
          1,
          "Enter value"
        );


        input = "";

        continue;
      }


      if (!pzemValid)
      {
        lcdLine(
          0,
          "PZEM unavailable"
        );


        lcdLine(
          1,
          "Try again"
        );


        delay(1500);


        lcd->clear();


        lcdLine(
          0,
          "Monthly kWh:"
        );


        lcdLine(
          1,
          "Enter value"
        );


        input = "";

        continue;
      }


      setMonthlyBudget(value);


      lcd->clear();


      lcdLine(
        0,
        "Budget Saved"
      );


      lcdLine(
        1,
        String(value, 1) +
        " kWh/month"
      );


      delay(2000);


      lcd->clear();

      return;
    }
  }
}


// ================================================================
//                     KEYPAD HANDLER
// ================================================================

void processKeypad()
{
  char key =
    keypad.getKey();


  if (!key)
  {
    return;
  }


  Serial.print(
    "Key pressed: "
  );

  Serial.println(key);


  // ------------------------------------------------
  // A = Set monthly budget
  // ------------------------------------------------

  if (key == 'A')
  {
    enterMonthlyBudget();

    return;
  }


  // ------------------------------------------------
  // B = Show budget information
  // ------------------------------------------------

  if (key == 'B')
  {
    lcd->clear();


    lcdLine(
      0,
      "Budget:" +
      String(monthlyBudget, 1)
    );


    lcdLine(
      1,
      "Daily:" +
      String(dailyBudget, 2)
    );


    delay(2500);


    lcd->clear();

    return;
  }


  // ------------------------------------------------
  // C = Show current consumption
  // ------------------------------------------------

  if (key == 'C')
  {
    lcd->clear();


    lcdLine(
      0,
      "Today:" +
      String(dailyEnergyUsed, 2)
    );


    lcdLine(
      1,
      "Month:" +
      String(monthlyEnergyUsed, 2)
    );


    delay(2500);


    lcd->clear();

    return;
  }


  // ------------------------------------------------
  // D = Manually restore load
  //
  // Only allowed if energy has not exceeded
  // the daily budget.
  // ------------------------------------------------

  if (key == 'D')
  {
    if (
      dailyEnergyUsed <
      dailyBudget
    )
    {
      loadCutOff =
        false;


      warningLevel =
        0;


      digitalWrite(
        RELAY_PIN,
        RELAY_ON_LEVEL
      );


      lcd->clear();


      lcdLine(
        0,
        "Load Restored"
      );


      lcdLine(
        1,
        "Budget Available"
      );


      delay(1500);


      lcd->clear();
    }
    else
    {
      lcd->clear();


      lcdLine(
        0,
        "Limit Reached"
      );


      lcdLine(
        1,
        "Load stays OFF"
      );


      delay(1500);


      lcd->clear();
    }
  }


  // ------------------------------------------------
  // # = force LCD next page
  // ------------------------------------------------

  if (key == '#')
  {
    lcdPage++;

    if (lcdPage >= 5)
    {
      lcdPage = 0;
    }


    updateLCD();
  }
}


// ================================================================
//                    SERIAL INFORMATION
// ================================================================

void printSystemStatus()
{
  Serial.println();
  Serial.println(
    "=============================="
  );

  Serial.println(
    "ENERGY MANAGEMENT STATUS"
  );

  Serial.println(
    "=============================="
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
    "Daily Used: %.3f kWh\n",
    dailyEnergyUsed
  );


  Serial.printf(
    "Daily Budget: %.3f kWh\n",
    dailyBudget
  );


  Serial.printf(
    "Daily Remaining: %.3f kWh\n",
    dailyEnergyRemaining
  );


  Serial.printf(
    "Monthly Used: %.3f kWh\n",
    monthlyEnergyUsed
  );


  Serial.printf(
    "Monthly Budget: %.3f kWh\n",
    monthlyBudget
  );


  Serial.printf(
    "Monthly Remaining: %.3f kWh\n",
    monthlyEnergyRemaining
  );


  Serial.printf(
    "Load: %s\n",
    loadCutOff ? "OFF" : "ON"
  );


  Serial.println(
    "=============================="
  );
}


// ================================================================
//                         SETUP
// ================================================================

void setup()
{
  Serial.begin(
    115200
  );


  delay(500);


  Serial.println();
  Serial.println(
    "Starting Energy Management System..."
  );


  // ------------------------------------------------
  // Relay
  // ------------------------------------------------

  pinMode(
    RELAY_PIN,
    OUTPUT
  );


  digitalWrite(
    RELAY_PIN,
    RELAY_ON_LEVEL
  );


  // ------------------------------------------------
  // Buzzer
  // ------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ------------------------------------------------
  // PZEM
  // ------------------------------------------------

  Serial2.begin(
    PZEM_BAUD,
    SERIAL_8N1,
    PZEM_RX_PIN,
    PZEM_TX_PIN
  );


  delay(100);


  // ------------------------------------------------
  // I2C
  // ------------------------------------------------

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  // ------------------------------------------------
  // LCD
  // ------------------------------------------------

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
    "Power Management"
  );


  lcdLine(
    1,
    "Starting..."
  );


  Serial.printf(
    "LCD address: 0x%02X\n",
    lcdAddr
  );


  delay(1500);


  // ------------------------------------------------
  // RTC
  // ------------------------------------------------

  if (!rtc.begin())
  {
    Serial.println(
      "RTC not detected."
    );


    lcdLine(
      0,
      "RTC ERROR"
    );


    lcdLine(
      1,
      "Check RTC"
    );


    delay(2000);
  }
  else
  {
    Serial.println(
      "RTC detected."
    );


    if (rtc.lostPower())
    {
      Serial.println(
        "RTC lost power."
      );


      /*
         Uncomment the following line ONLY when
         setting the RTC for the first time.

         rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      */
    }
  }


  // ------------------------------------------------
  // SD CARD
  // ------------------------------------------------

  initializeSD();


  // ------------------------------------------------
  // Preferences
  // ------------------------------------------------

  loadSettings();


  // ------------------------------------------------
  // Initial PZEM read
  // ------------------------------------------------

  uint8_t got = 0;


  PzemStatus st =
    pzemRead(
      data,
      got
    );


  if (st == PZEM_OK)
  {
    pzemValid =
      true;


    pzemFailCount =
      0;


    Serial.println(
      "Initial PZEM reading successful."
    );
  }
  else
  {
    pzemValid =
      false;


    Serial.println(
      "Initial PZEM reading failed."
    );
  }


  // ------------------------------------------------
  // RTC date
  // ------------------------------------------------

  if (rtc.begin())
  {
    DateTime now =
      rtc.now();


    /*
       First-time initialization.

       If no valid stored date exists,
       start a new energy cycle.
    */

    if (
      currentYear == 0 ||
      currentMonth == 0 ||
      currentDay == 0
    )
    {
      currentYear =
        now.year();

      currentMonth =
        now.month();

      currentDay =
        now.day();


      if (pzemValid)
      {
        monthlyStartEnergy =
          data.energy;

        dailyStartEnergy =
          data.energy;
      }


      saveSettings();
    }
  }


  calculateDailyBudget();


  // ------------------------------------------------
  // Startup screen
  // ------------------------------------------------

  lcd->clear();


  if (pzemValid)
  {
    lcdLine(
      0,
      "System Ready"
    );


    lcdLine(
      1,
      "Load ON"
    );
  }
  else
  {
    lcdLine(
      0,
      "PZEM Error"
    );


    lcdLine(
      1,
      "Check PZEM"
    );
  }


  delay(1500);


  lcd->clear();


  Serial.println();
  Serial.println(
    "System ready."
  );


  Serial.println(
    "Keypad:"
  );


  Serial.println(
    "A = Set monthly budget"
  );


  Serial.println(
    "B = Show budget"
  );


  Serial.println(
    "C = Show consumption"
  );


  Serial.println(
    "D = Restore load"
  );


  Serial.println(
    "# = Next LCD page"
  );


  Serial.println(
    "Serial 'r' = Reset PZEM energy"
  );
}


// ================================================================
//                         MAIN LOOP
// ================================================================

void loop()
{
  unsigned long now =
    millis();


  // ==============================================================
  //                    SERIAL COMMAND
  // ==============================================================

  if (Serial.available())
  {
    char command =
      Serial.read();


    if (command == 'r')
    {
      pzemResetEnergy();


      /*
         Since the PZEM counter was reset,
         restart the energy baseline.
      */

      if (pzemValid)
      {
        monthlyStartEnergy =
          data.energy;

        dailyStartEnergy =
          data.energy;


        monthlyEnergyUsed =
          0;

        dailyEnergyUsed =
          0;


        saveSettings();
      }
    }


    if (command == 's')
    {
      printSystemStatus();
    }
  }


  // ==============================================================
  //                    PZEM READING
  // ==============================================================

  if (
    now - lastPzemRead >=
    PZEM_INTERVAL
  )
  {
    lastPzemRead =
      now;


    uint8_t got = 0;


    PzemStatus st =
      pzemRead(
        data,
        got
      );


    if (st == PZEM_OK)
    {
      pzemValid =
        true;


      pzemFailCount =
        0;


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
        pzemValid =
          false;
      }


      switch (st)
      {
        case PZEM_NO_REPLY:

          Serial.println(
            "PZEM: No reply."
          );

          break;


        case PZEM_BAD_FRAME:

          Serial.printf(
            "PZEM: Bad frame. Bytes=%u\n",
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
  }


  // ==============================================================
  //                    DATE MANAGEMENT
  // ==============================================================

  if (
    now - lastBudgetCheck >=
    BUDGET_INTERVAL
  )
  {
    lastBudgetCheck =
      now;


    checkDateChange();


    calculateEnergyUsage();


    processWarnings();
  }


  // ==============================================================
  //                    SD LOGGING
  // ==============================================================

  if (
    now - lastLog >=
    LOG_INTERVAL
  )
  {
    lastLog =
      now;


    logEnergyToSD();
  }


  // ==============================================================
  //                    KEYPAD
  // ==============================================================

  processKeypad();


  // ==============================================================
  //                    LCD UPDATE
  // ==============================================================

  if (
    now - lastLcdUpdate >=
    LCD_INTERVAL
  )
  {
    lastLcdUpdate =
      now;


    updateLCD();
  }


  // ==============================================================
  //                    LCD PAGE CHANGE
  // ==============================================================

  if (
    now - lastPageChange >=
    PAGE_INTERVAL
  )
  {
    lastPageChange =
      now;


    lcdPage++;


    if (lcdPage >= 5)
    {
      lcdPage = 0;
    }


    updateLCD();
  }


  delay(5);
}