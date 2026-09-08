/* mkodarafem@gmail.com
   ============================================================
   FINAL PILL DISPENSER
   ESP32 DevKit V1 - 30 Pin

   Bluetooth-controlled pill dispensing schedules
   Maximum schedules: 10

   Bluetooth commands:

   SET,0,15,30,5
   SET,0,15,30,5,2,9,25,16,5,20,10,0

   CLEAR
   GET

   Manual stepper jog commands:

   <n   -> rotate LEFT  by n steps
   >n   -> rotate RIGHT by n steps

   Set RTC date/time:

   TIME,year,month,day,hour,minute,second
   TIME,2026,9,8,14,30,0

   ESP32 responses:

   OK,1
   OK,3
   OK,CLEAR
   OK,LEFT
   OK,RIGHT
   OK,TIME
   SCHEDULES,3,0,15,30,5,2,9,25,16,5,5,20,10,0

   Day:
   0 = Sunday
   1 = Monday
   2 = Tuesday
   3 = Wednesday
   4 = Thursday
   5 = Friday
   6 = Saturday

   ============================================================
   HARDWARE
   ============================================================

   ESP32 DevKit V1
   DS3231 RTC
   16x2 I2C LCD
   28BYJ-48 Stepper Motor
   ULN2003 Driver
   Confirm Button
   Buzzer
   LED

   ============================================================
   PIN ALLOCATION
   ============================================================

   STEPPER / ULN2003

   ESP32 GPIO26 -> ULN2003 IN1
   ESP32 GPIO27 -> ULN2003 IN2
   ESP32 GPIO14 -> ULN2003 IN3
   ESP32 GPIO13 -> ULN2003 IN4

   I2C

   ESP32 GPIO21 -> SDA -> LCD + DS3231
   ESP32 GPIO22 -> SCL -> LCD + DS3231

   CONFIRM BUTTON

   ESP32 GPIO4 -> conf_btn

   BUZZER

   ESP32 GPIO25 -> Buzzer transistor circuit

   LED

   ESP32 GPIO2 -> LED

   ============================================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <AccelStepper.h>
#include "BluetoothSerial.h"
#include <Preferences.h>


// ============================================================
// LCD
// ============================================================

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS    2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);


// ============================================================
// LCD CUSTOM CHARACTERS
// ============================================================

// Custom character slot used for the Bluetooth icon
// (lcd.createChar only supports slots 0-7 on the HD44780)

#define BLUETOOTH_ICON_SLOT 0

// Bluetooth glyph, 5x8 pixels, for lcd.createChar()

byte bluetoothIcon[8] =
{
  0b00100,
  0b10110,
  0b01101,
  0b00100,
  0b01101,
  0b10110,
  0b00100,
  0b00000
};

// Column/row used to show the Bluetooth icon on the LCD

#define BLUETOOTH_ICON_COL 15
#define BLUETOOTH_ICON_ROW 0


// ============================================================
// RTC
// ============================================================

RTC_DS3231 rtc;


// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;

#define BLUETOOTH_DEVICE_NAME "PillDispenser"


// ============================================================
// PREFERENCES / NON-VOLATILE STORAGE
// ============================================================

Preferences preferences;

#define MAX_SCHEDULES 10


// ============================================================
// ESP32 PIN DEFINITIONS
// ============================================================

// -------------------- ULN2003 / 28BYJ-48 STEPPER --------------------

#define motorPin1  26
#define motorPin2  27
#define motorPin3  14
#define motorPin4  13


// AccelStepper interface type
// 8 = 4-wire motor in half-step mode

#define MotorInterfaceType 8


// AccelStepper pin sequence:
// IN1 -> IN3 -> IN2 -> IN4

AccelStepper stepper(
  MotorInterfaceType,
  motorPin1,
  motorPin3,
  motorPin2,
  motorPin4
);


// ============================================================
// I2C PIN DEFINITIONS
// ============================================================

#define I2C_SDA 21
#define I2C_SCL 22


// ============================================================
// CONFIRM BUTTON
// ============================================================

#define CONFIRM_BUTTON_PIN 4


// ============================================================
// BUZZER
// ============================================================

#define BUZZER_PIN 25


// ============================================================
// LED
// ============================================================

#define LED_PIN 2


// ============================================================
// PILL SCHEDULE STRUCTURE
// ============================================================

struct PillSchedule
{
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};


// Array containing all schedules

PillSchedule schedules[MAX_SCHEDULES];


// Number of schedules currently stored

uint8_t scheduleCount = 0;


// ============================================================
// DISPENSING TRACKING
// ============================================================

// Prevents the same schedule from triggering repeatedly
// during the same second.

int lastTriggeredYear = -1;
int lastTriggeredMonth = -1;
int lastTriggeredDay = -1;
int lastTriggeredSchedule = -1;


// ============================================================
// VARIABLES
// ============================================================

int buttonState = HIGH;


// Bluetooth receive buffer

String bluetoothBuffer = "";


// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void updateRTC();

void updateLCD();

void processBluetooth();

void processBluetoothCommand(String command);

bool processSetCommand(String command);

bool processMoveCommand(String command);

bool processTimeCommand(String command);

bool validateSchedule(PillSchedule schedule);

void saveSchedules();

void loadSchedules();

void clearSchedules();

void sendSchedules();

void sendBluetoothError(const char *errorMessage);

void sendBluetoothOK(const char *message);

bool isScheduleTime(const PillSchedule &schedule,
                    const DateTime &rtcTime);

bool alreadyTriggered(int scheduleIndex,
                      const DateTime &rtcTime);

void markAsTriggered(int scheduleIndex,
                     const DateTime &rtcTime);

void checkDispensingSchedules();

void dispensePill();

void rotatePill();

void rotateOver();

void moveStepperSteps(long steps);

void rotateLeft(long steps);

void rotateRight(long steps);

void printSchedulesToSerial();


// ============================================================
// INPUT VALIDATION STRUCTURE
// ============================================================

typedef struct minMax_t
{
  int minimum;
  int maximum;
};


// ============================================================
// CHECK USER INPUT
// ============================================================

bool checkInput(const int value, const minMax_t minMax)
{
  if ((value >= minMax.minimum) &&
      (value <= minMax.maximum))
  {
    return true;
  }

  Serial.print(value);
  Serial.print(" is out of range ");
  Serial.print(minMax.minimum);
  Serial.print(" - ");
  Serial.println(minMax.maximum);

  return false;
}


// ============================================================
// UPDATE RTC
// ============================================================

void updateRTC()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Edit Mode...");


  const char txt[6][15] =
  {
    "year [4-digit]",
    "month [1~12]",
    "day [1~31]",
    "hours [0~23]",
    "minutes [0~59]",
    "seconds [0~59]"
  };


  const minMax_t minMax[] =
  {
    {2000, 9999},
    {1, 12},
    {1, 31},
    {0, 23},
    {0, 59},
    {0, 59}
  };


  String str = "";

  long newDate[6];

  DateTime newDateTime;


  // ----------------------------------------------------------
  // Outer loop
  // ----------------------------------------------------------

  while (1)
  {
    // Clear serial buffer

    while (Serial.available())
    {
      Serial.read();
    }


    // --------------------------------------------------------
    // Get all six values
    // --------------------------------------------------------

    for (int i = 0; i < 6; i++)
    {
      while (1)
      {
        Serial.print("Enter ");
        Serial.print(txt[i]);
        Serial.print(" (or -1 to abort) : ");


        // Wait for input

        while (!Serial.available())
        {
          ;
        }


        // Read input

        str = Serial.readString();
        str.trim();


        // ----------------------------------------------------
        // Abort
        // ----------------------------------------------------

        if (str == "-1")
        {
          Serial.println("\nABORTED");

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Edit Aborted");

          delay(1000);

          return;
        }


        // Convert input to number

        newDate[i] = str.toInt();


        // Validate

        if (checkInput(newDate[i], minMax[i]))
        {
          break;
        }
      }


      Serial.println(newDate[i]);
    }


    // --------------------------------------------------------
    // Create DateTime
    // --------------------------------------------------------

    newDateTime = DateTime(
      newDate[0],
      newDate[1],
      newDate[2],
      newDate[3],
      newDate[4],
      newDate[5]
    );


    // Check complete date/time

    if (newDateTime.isValid())
    {
      break;
    }


    Serial.println(
      "Date/time entered was invalid, please try again."
    );
  }


  // ----------------------------------------------------------
  // Update RTC
  // ----------------------------------------------------------

  rtc.adjust(newDateTime);

  Serial.println("RTC Updated!");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RTC Updated");

  delay(1000);

  lcd.clear();
}


// ============================================================
// UPDATE LCD
// ============================================================

void updateLCD()
{
  DateTime rtcTime = rtc.now();

  char timeBuffer[] = "hh:mm:ss";


  lcd.setCursor(0, 0);
  lcd.print("Time ");

  lcd.setCursor(5, 0);
  lcd.print(rtcTime.toString(timeBuffer));


  // ----------------------------------------------------------
  // Bluetooth connection icon
  //
  // Show the custom Bluetooth glyph only while a client is
  // connected. When no client is connected, blank the spot.
  // ----------------------------------------------------------

  lcd.setCursor(BLUETOOTH_ICON_COL, BLUETOOTH_ICON_ROW);

  if (SerialBT.hasClient())
  {
    lcd.write(byte(BLUETOOTH_ICON_SLOT));
  }
  else
  {
    lcd.print(" ");
  }


  lcd.setCursor(0, 1);

  lcd.print("Schedules: ");

  lcd.print(scheduleCount);

  // Clear any remaining characters

  lcd.print("     ");
}


// ============================================================
// LOAD SCHEDULES FROM ESP32 NVS
// ============================================================

void loadSchedules()
{
  preferences.begin("pilldata", true);

  scheduleCount =
    preferences.getUChar("count", 0);


  if (scheduleCount > MAX_SCHEDULES)
  {
    scheduleCount = 0;
  }


  for (int i = 0; i < scheduleCount; i++)
  {
    String keyDay =
      "d" + String(i);

    String keyHour =
      "h" + String(i);

    String keyMinute =
      "m" + String(i);

    String keySecond =
      "s" + String(i);


    schedules[i].day =
      preferences.getUChar(keyDay.c_str(), 0);

    schedules[i].hour =
      preferences.getUChar(keyHour.c_str(), 0);

    schedules[i].minute =
      preferences.getUChar(keyMinute.c_str(), 0);

    schedules[i].second =
      preferences.getUChar(keySecond.c_str(), 0);
  }


  preferences.end();


  Serial.print("Loaded ");
  Serial.print(scheduleCount);
  Serial.println(" schedules from memory.");
}


// ============================================================
// SAVE SCHEDULES TO ESP32 NVS
// ============================================================

void saveSchedules()
{
  preferences.begin("pilldata", false);


  // First remove old schedule data

  preferences.clear();


  preferences.putUChar(
    "count",
    scheduleCount
  );


  for (int i = 0; i < scheduleCount; i++)
  {
    String keyDay =
      "d" + String(i);

    String keyHour =
      "h" + String(i);

    String keyMinute =
      "m" + String(i);

    String keySecond =
      "s" + String(i);


    preferences.putUChar(
      keyDay.c_str(),
      schedules[i].day
    );

    preferences.putUChar(
      keyHour.c_str(),
      schedules[i].hour
    );

    preferences.putUChar(
      keyMinute.c_str(),
      schedules[i].minute
    );

    preferences.putUChar(
      keySecond.c_str(),
      schedules[i].second
    );
  }


  preferences.end();


  Serial.println("Schedules saved to memory.");
}


// ============================================================
// CLEAR ALL SCHEDULES
// ============================================================

void clearSchedules()
{
  scheduleCount = 0;


  preferences.begin("pilldata", false);

  preferences.clear();

  preferences.putUChar("count", 0);

  preferences.end();


  Serial.println("All schedules cleared.");
}


// ============================================================
// VALIDATE ONE SCHEDULE
// ============================================================

bool validateSchedule(PillSchedule schedule)
{
  if (schedule.day > 6)
  {
    sendBluetoothError("INVALID_DAY");

    return false;
  }


  if (schedule.hour > 23)
  {
    sendBluetoothError("INVALID_HOUR");

    return false;
  }


  if (schedule.minute > 59)
  {
    sendBluetoothError("INVALID_MINUTE");

    return false;
  }


  if (schedule.second > 59)
  {
    sendBluetoothError("INVALID_SECOND");

    return false;
  }


  return true;
}


// ============================================================
// PROCESS BLUETOOTH DATA
// ============================================================

void processBluetooth()
{
  while (SerialBT.available())
  {
    char incomingChar = SerialBT.read();

    // --------------------------------------------------------
    // Ignore NULL and other non-printable characters
    // --------------------------------------------------------

    if (incomingChar < 32 &&
        incomingChar != '\r' &&
        incomingChar != '\n')
    {
      continue;
    }


    // --------------------------------------------------------
    // Actual newline / carriage return
    // --------------------------------------------------------

    if (incomingChar == '\n' ||
        incomingChar == '\r')
    {
      if (bluetoothBuffer.length() > 0)
      {
        bluetoothBuffer.trim();

        if (bluetoothBuffer.length() > 0)
        {
          processBluetoothCommand(
            bluetoothBuffer
          );
        }

        bluetoothBuffer = "";
      }

      continue;
    }


    // --------------------------------------------------------
    // Add normal character
    // --------------------------------------------------------

    bluetoothBuffer += incomingChar;


    // --------------------------------------------------------
    // Implicit command boundary for jog commands
    //
    // Some Bluetooth apps (e.g. certain MIT App Inventor
    // projects) send "<n" / ">n" jog commands back-to-back
    // with no delimiter between them, so the ESP32's receive
    // buffer would otherwise end up as one long unparseable
    // string like "<10<10>5CLEAR".
    //
    // A jog command's digits only ever end when a
    // non-digit character shows up next, so that next
    // character (another '<'/'>' or the first letter of a
    // keyword command) is treated as the start of a brand
    // new command, and whatever was buffered before it is
    // processed immediately.
    // --------------------------------------------------------

    if (bluetoothBuffer.length() > 1)
    {
      char firstChar =
        bluetoothBuffer.charAt(0);

      if (firstChar == '<' ||
          firstChar == '>')
      {
        char lastChar =
          bluetoothBuffer.charAt(
            bluetoothBuffer.length() - 1
          );

        if (!isDigit(lastChar))
        {
          String previousCommand =
            bluetoothBuffer.substring(
              0,
              bluetoothBuffer.length() - 1
            );

          previousCommand.trim();

          if (previousCommand.length() > 0)
          {
            processBluetoothCommand(
              previousCommand
            );
          }

          // Keep the character that triggered the split -
          // it's the start of the next command

          bluetoothBuffer =
            String(lastChar);
        }
      }
    }


    // --------------------------------------------------------
    // Handle literal "\n"
    //
    // Some Android terminal apps send the two characters:
    //
    // \
    // n
    //
    // instead of an actual newline.
    // --------------------------------------------------------

    if (bluetoothBuffer.endsWith("\\n"))
    {
      // Remove the literal "\n"

      bluetoothBuffer.remove(
        bluetoothBuffer.length() - 2
      );

      bluetoothBuffer.trim();


      if (bluetoothBuffer.length() > 0)
      {
        processBluetoothCommand(
          bluetoothBuffer
        );
      }


      bluetoothBuffer = "";
    }


    // --------------------------------------------------------
    // Prevent oversized commands
    // --------------------------------------------------------

    if (bluetoothBuffer.length() > 200)
    {
      bluetoothBuffer = "";

      sendBluetoothError(
        "COMMAND_TOO_LONG"
      );
    }
  }
}

// ============================================================
// PROCESS BLUETOOTH COMMAND
// ============================================================

void processBluetoothCommand(String command)
{
  command.trim();


  Serial.print("Bluetooth RX: ");
  Serial.println(command);


  // ----------------------------------------------------------
  // SET
  // ----------------------------------------------------------

  if (command.startsWith("SET,") ||
      command == "SET")
  {
    processSetCommand(command);

    return;
  }


  // ----------------------------------------------------------
  // CLEAR
  // ----------------------------------------------------------

  if (command == "CLEAR")
  {
    clearSchedules();

    sendBluetoothOK("CLEAR");

    return;
  }


  // ----------------------------------------------------------
  // GET
  // ----------------------------------------------------------

  if (command == "GET")
  {
    sendSchedules();

    return;
  }


  // ----------------------------------------------------------
  // MANUAL STEPPER JOG
  //
  // "<n" rotates LEFT  by n steps
  // ">n" rotates RIGHT by n steps
  // ----------------------------------------------------------

  if (command.startsWith("<") ||
      command.startsWith(">"))
  {
    processMoveCommand(command);

    return;
  }


  // ----------------------------------------------------------
  // TIME
  // ----------------------------------------------------------

  if (command.startsWith("TIME,"))
  {
    processTimeCommand(command);

    return;
  }


  // ----------------------------------------------------------
  // Unknown command
  // ----------------------------------------------------------

  sendBluetoothError(
    "INVALID_COMMAND"
  );
}


// ============================================================
// PROCESS SET COMMAND
// ============================================================

bool processSetCommand(String command)
{
  /*
     Expected:

     SET,day,hour,minute,second,...

     Number of values after SET must be:

     4
     8
     12
     16
     ...

     Maximum:

     40 values
     = 10 schedules
  */


  // Temporary array.
  // We don't change the active schedule until
  // the entire command has been validated.

  PillSchedule newSchedules[MAX_SCHEDULES];

  int newScheduleCount = 0;


  // ----------------------------------------------------------
  // Remove "SET,"
  // ----------------------------------------------------------

  command.remove(0, 4);


  // If nothing follows SET

  if (command.length() == 0)
  {
    // SET with no schedules means clear schedule

    scheduleCount = 0;

    saveSchedules();

    sendBluetoothOK("0");

    return true;
  }


  // ----------------------------------------------------------
  // Parse values
  // ----------------------------------------------------------

  int values[40];

  int valueCount = 0;

  int startIndex = 0;


  while (startIndex < command.length())
  {
    int commaIndex =
      command.indexOf(',', startIndex);


    String valueString;


    if (commaIndex == -1)
    {
      valueString =
        command.substring(startIndex);

      startIndex = command.length();
    }
    else
    {
      valueString =
        command.substring(
          startIndex,
          commaIndex
        );

      startIndex =
        commaIndex + 1;
    }


    valueString.trim();


    if (valueString.length() == 0)
    {
      sendBluetoothError(
        "INVALID_DATA"
      );

      return false;
    }


    // Check maximum values

    if (valueCount >= 40)
    {
      sendBluetoothError(
        "MAX_10"
      );

      return false;
    }


    // Make sure the value is actually numeric

    for (unsigned int i = 0;
         i < valueString.length();
         i++)
    {
      char c = valueString.charAt(i);


      if (!isDigit(c))
      {
        sendBluetoothError(
          "INVALID_DATA"
        );

        return false;
      }
    }


    values[valueCount] =
      valueString.toInt();


    valueCount++;
  }


  // ----------------------------------------------------------
  // Number of values must be divisible by 4
  // ----------------------------------------------------------

  if (valueCount % 4 != 0)
  {
    sendBluetoothError(
      "INCOMPLETE_SCHEDULE"
    );

    return false;
  }


  // ----------------------------------------------------------
  // Determine number of schedules
  // ----------------------------------------------------------

  newScheduleCount =
    valueCount / 4;


  if (newScheduleCount > MAX_SCHEDULES)
  {
    sendBluetoothError(
      "MAX_10"
    );

    return false;
  }


  // ----------------------------------------------------------
  // Create temporary schedules
  // ----------------------------------------------------------

  for (int i = 0;
       i < newScheduleCount;
       i++)
  {
    newSchedules[i].day =
      values[(i * 4) + 0];

    newSchedules[i].hour =
      values[(i * 4) + 1];

    newSchedules[i].minute =
      values[(i * 4) + 2];

    newSchedules[i].second =
      values[(i * 4) + 3];


    // Validate schedule

    if (!validateSchedule(
          newSchedules[i]))
    {
      return false;
    }
  }


  // ----------------------------------------------------------
  // Everything is valid
  // Replace existing schedules
  // ----------------------------------------------------------

  scheduleCount =
    newScheduleCount;


  for (int i = 0;
       i < scheduleCount;
       i++)
  {
    schedules[i] =
      newSchedules[i];
  }


  // Save permanently

  saveSchedules();


  // Reset trigger tracking

  lastTriggeredSchedule = -1;


  // ----------------------------------------------------------
  // Briefly show a "Schedule Received" message on the LCD
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Schedule");

  lcd.setCursor(0, 1);
  lcd.print("Received!");

  delay(2000);

  lcd.clear();


  // Send confirmation

  sendBluetoothOK(
    String(scheduleCount).c_str()
  );


  // Print to USB Serial

  printSchedulesToSerial();


  return true;
}


// ============================================================
// PROCESS MANUAL STEPPER MOVE COMMAND
// ============================================================

bool processMoveCommand(String command)
{
  /*
     Expected:

     <n   -> rotate LEFT  by n steps
     >n   -> rotate RIGHT by n steps
  */

  char directionChar =
    command.charAt(0);


  String numberPart =
    command.substring(1);

  numberPart.trim();


  // ----------------------------------------------------------
  // Must have a number after the direction character
  // ----------------------------------------------------------

  if (numberPart.length() == 0)
  {
    sendBluetoothError(
      "INVALID_DATA"
    );

    return false;
  }


  // ----------------------------------------------------------
  // Make sure the number is actually numeric
  // ----------------------------------------------------------

  for (unsigned int i = 0;
       i < numberPart.length();
       i++)
  {
    char c = numberPart.charAt(i);


    if (!isDigit(c))
    {
      sendBluetoothError(
        "INVALID_DATA"
      );

      return false;
    }
  }


  long steps =
    numberPart.toInt();


  if (steps <= 0)
  {
    sendBluetoothError(
      "INVALID_STEPS"
    );

    return false;
  }


  // ----------------------------------------------------------
  // Perform the move
  // ----------------------------------------------------------

  if (directionChar == '<')
  {
    rotateLeft(steps);

    sendBluetoothOK("LEFT");
  }
  else
  {
    rotateRight(steps);

    sendBluetoothOK("RIGHT");
  }


  return true;
}


// ============================================================
// PROCESS RTC TIME-SET COMMAND
// ============================================================

bool processTimeCommand(String command)
{
  /*
     Expected:

     TIME,year,month,day,hour,minute,second

     Example:

     TIME,2026,9,8,14,30,0
  */


  // ----------------------------------------------------------
  // Remove "TIME,"
  // ----------------------------------------------------------

  command.remove(0, 5);


  // ----------------------------------------------------------
  // Parse the 6 comma-separated values
  // ----------------------------------------------------------

  int values[6];

  int valueCount = 0;

  int startIndex = 0;


  while (startIndex < command.length())
  {
    int commaIndex =
      command.indexOf(',', startIndex);


    String valueString;


    if (commaIndex == -1)
    {
      valueString =
        command.substring(startIndex);

      startIndex = command.length();
    }
    else
    {
      valueString =
        command.substring(
          startIndex,
          commaIndex
        );

      startIndex =
        commaIndex + 1;
    }


    valueString.trim();


    if (valueString.length() == 0)
    {
      sendBluetoothError(
        "INVALID_DATA"
      );

      return false;
    }


    // Check maximum values

    if (valueCount >= 6)
    {
      sendBluetoothError(
        "INVALID_TIME"
      );

      return false;
    }


    // Make sure the value is actually numeric

    for (unsigned int i = 0;
         i < valueString.length();
         i++)
    {
      char c = valueString.charAt(i);


      if (!isDigit(c))
      {
        sendBluetoothError(
          "INVALID_DATA"
        );

        return false;
      }
    }


    values[valueCount] =
      valueString.toInt();


    valueCount++;
  }


  // ----------------------------------------------------------
  // Must have exactly 6 values
  // ----------------------------------------------------------

  if (valueCount != 6)
  {
    sendBluetoothError(
      "INVALID_TIME"
    );

    return false;
  }


  int year   = values[0];
  int month  = values[1];
  int day    = values[2];
  int hour   = values[3];
  int minute = values[4];
  int second = values[5];


  // ----------------------------------------------------------
  // Validate ranges
  // ----------------------------------------------------------

  if (year < 2000 || year > 9999)
  {
    sendBluetoothError("INVALID_YEAR");

    return false;
  }

  if (month < 1 || month > 12)
  {
    sendBluetoothError("INVALID_MONTH");

    return false;
  }

  if (day < 1 || day > 31)
  {
    sendBluetoothError("INVALID_DAY");

    return false;
  }

  if (hour > 23)
  {
    sendBluetoothError("INVALID_HOUR");

    return false;
  }

  if (minute > 59)
  {
    sendBluetoothError("INVALID_MINUTE");

    return false;
  }

  if (second > 59)
  {
    sendBluetoothError("INVALID_SECOND");

    return false;
  }


  // ----------------------------------------------------------
  // Build and validate the DateTime
  // ----------------------------------------------------------

  DateTime newDateTime(
    year,
    month,
    day,
    hour,
    minute,
    second
  );


  if (!newDateTime.isValid())
  {
    sendBluetoothError(
      "INVALID_TIME"
    );

    return false;
  }


  // ----------------------------------------------------------
  // Update the RTC
  // ----------------------------------------------------------

  rtc.adjust(newDateTime);


  Serial.println(
    "RTC updated via Bluetooth."
  );


  sendBluetoothOK("TIME");


  // ----------------------------------------------------------
  // Briefly confirm on the LCD
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("RTC Updated");

  delay(1000);

  lcd.clear();


  return true;
}


// ============================================================
// SEND BLUETOOTH ERROR
// ============================================================

void sendBluetoothError(
  const char *errorMessage
)
{
  SerialBT.print("ERROR,");
  SerialBT.println(errorMessage);


  Serial.print("Bluetooth TX: ERROR,");
  Serial.println(errorMessage);
}


// ============================================================
// SEND BLUETOOTH OK
// ============================================================

void sendBluetoothOK(
  const char *message
)
{
  SerialBT.print("OK,");
  SerialBT.println(message);


  Serial.print("Bluetooth TX: OK,");
  Serial.println(message);
}


// ============================================================
// SEND ALL SCHEDULES
// ============================================================

void sendSchedules()
{
  /*
     Example:

     SCHEDULES,3,
     0,15,30,5,
     2,9,25,16,
     5,20,10,0
  */


  SerialBT.print("SCHEDULES,");
  SerialBT.print(scheduleCount);


  for (int i = 0;
       i < scheduleCount;
       i++)
  {
    SerialBT.print(",");

    SerialBT.print(
      schedules[i].day
    );

    SerialBT.print(",");

    SerialBT.print(
      schedules[i].hour
    );

    SerialBT.print(",");

    SerialBT.print(
      schedules[i].minute
    );

    SerialBT.print(",");

    SerialBT.print(
      schedules[i].second
    );
  }


  SerialBT.println();


  Serial.print("Bluetooth TX: SCHEDULES,");
  Serial.println(scheduleCount);
}


// ============================================================
// PRINT SCHEDULES TO USB SERIAL
// ============================================================

void printSchedulesToSerial()
{
  Serial.println();
  Serial.println(
    "========== CURRENT SCHEDULES =========="
  );


  if (scheduleCount == 0)
  {
    Serial.println("No schedules stored.");
  }


  for (int i = 0;
       i < scheduleCount;
       i++)
  {
    Serial.print("#");
    Serial.print(i + 1);

    Serial.print("  Day=");
    Serial.print(schedules[i].day);

    Serial.print("  Time=");

    if (schedules[i].hour < 10)
      Serial.print("0");

    Serial.print(
      schedules[i].hour
    );

    Serial.print(":");

    if (schedules[i].minute < 10)
      Serial.print("0");

    Serial.print(
      schedules[i].minute
    );

    Serial.print(":");

    if (schedules[i].second < 10)
      Serial.print("0");

    Serial.println(
      schedules[i].second
    );
  }


  Serial.println(
    "======================================="
  );
  Serial.println();
}


// ============================================================
// CHECK IF CURRENT TIME MATCHES SCHEDULE
// ============================================================

bool isScheduleTime(
  const PillSchedule &schedule,
  const DateTime &rtcTime
)
{
  // RTClib:
  // 0 = Sunday
  // 1 = Monday
  // ...
  // 6 = Saturday

  if (rtcTime.dayOfTheWeek() !=
      schedule.day)
  {
    return false;
  }


  if (rtcTime.hour() !=
      schedule.hour)
  {
    return false;
  }


  if (rtcTime.minute() !=
      schedule.minute)
  {
    return false;
  }


  if (rtcTime.second() !=
      schedule.second)
  {
    return false;
  }


  return true;
}


// ============================================================
// CHECK WHETHER THIS EVENT WAS ALREADY TRIGGERED
// ============================================================

bool alreadyTriggered(
  int scheduleIndex,
  const DateTime &rtcTime
)
{
  if (lastTriggeredSchedule ==
      scheduleIndex &&
      lastTriggeredYear ==
      rtcTime.year() &&
      lastTriggeredMonth ==
      rtcTime.month() &&
      lastTriggeredDay ==
      rtcTime.day())
  {
    return true;
  }


  return false;
}


// ============================================================
// MARK EVENT AS TRIGGERED
// ============================================================

void markAsTriggered(
  int scheduleIndex,
  const DateTime &rtcTime
)
{
  lastTriggeredSchedule =
    scheduleIndex;

  lastTriggeredYear =
    rtcTime.year();

  lastTriggeredMonth =
    rtcTime.month();

  lastTriggeredDay =
    rtcTime.day();
}


// ============================================================
// CHECK ALL DISPENSING SCHEDULES
// ============================================================

void checkDispensingSchedules()
{
  DateTime rtcTime =
    rtc.now();


  for (int i = 0;
       i < scheduleCount;
       i++)
  {
    if (isScheduleTime(
          schedules[i],
          rtcTime))
    {
      if (!alreadyTriggered(
            i,
            rtcTime))
      {
        // Mark first so the event cannot
        // trigger again during the same event.

        markAsTriggered(
          i,
          rtcTime
        );


        Serial.println();
        Serial.println(
          "================================"
        );

        Serial.print(
          "DISPENSING SCHEDULE #"
        );

        Serial.println(i + 1);


        Serial.println(
          "================================"
        );


        dispensePill();
      }
    }
  }
}


// ============================================================
// DISPENSE PILL
// ============================================================

void dispensePill()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Dispensing...");


  // ----------------------------------------------------------
  // Rotate the pill dispenser
  // ----------------------------------------------------------

  rotatePill();


  // ----------------------------------------------------------
  // Pill has been dispensed
  // Start alarm
  // ----------------------------------------------------------

  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);


  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Pills Ready");

  lcd.setCursor(0, 1);
  lcd.print("Press Button");


  Serial.println("Pills ready.");
  Serial.println("Waiting for confirmation...");


  // ----------------------------------------------------------
  // Keep buzzer ON until confirm button is pressed
  // ----------------------------------------------------------

  while (digitalRead(CONFIRM_BUTTON_PIN) == HIGH)
  {
    // Keep buzzer sounding

    digitalWrite(BUZZER_PIN, HIGH);

    // Keep LED ON

    digitalWrite(LED_PIN, HIGH);

    delay(20);
  }


  // ----------------------------------------------------------
  // Button pressed
  // ----------------------------------------------------------

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);


  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Pills Taken");


  Serial.println("Confirm button pressed.");
  Serial.println("Pills taken.");


  // ----------------------------------------------------------
  // Wait for button to be released
  // ----------------------------------------------------------

  while (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
  {
    delay(20);
  }


  delay(200);

  lcd.clear();
}

// ============================================================
// STEPPER PILL DISPENSING
// ============================================================

void rotatePill()
{
  // ----------------------------------------------------------
  // Enable stepper
  // ----------------------------------------------------------

  stepper.enableOutputs();


  // Start position

  stepper.setCurrentPosition(0);


  // ----------------------------------------------------------
  // Rotate 2048 steps
  // ----------------------------------------------------------

  while (
    stepper.currentPosition() != 2048
  )
  {
    stepper.setSpeed(500);

    stepper.runSpeed();
  }


  // ----------------------------------------------------------
  // Disable motor outputs
  // ----------------------------------------------------------

  stepper.disableOutputs();
}


// ============================================================
// CONFIRM / OVERRIDE ROTATION
// ============================================================

void rotateOver()
{
  stepper.enableOutputs();


  stepper.setCurrentPosition(0);


  while (
    stepper.currentPosition() != 2048
  )
  {
    stepper.setSpeed(500);

    stepper.runSpeed();
  }


  stepper.disableOutputs();


  lcd.setCursor(0, 1);

  lcd.print("Timer Overridden");

  digitalWrite(
    LED_PIN,
    HIGH
  );
}


// ============================================================
// MANUAL STEPPER JOG (LEFT / RIGHT BY N STEPS)
// ============================================================

// Moves the stepper by a signed number of steps relative to
// its current position. Positive values move RIGHT,
// negative values move LEFT.

void moveStepperSteps(long steps)
{
  stepper.enableOutputs();

  stepper.setCurrentPosition(0);

  int direction =
    (steps >= 0) ? 1 : -1;


  while (
    stepper.currentPosition() != steps
  )
  {
    stepper.setSpeed(500 * direction);

    stepper.runSpeed();
  }


  stepper.disableOutputs();
}


// Rotate the stepper LEFT by the given number of steps

void rotateLeft(long steps)
{
  moveStepperSteps(-steps);
}


// Rotate the stepper RIGHT by the given number of steps

void rotateRight(long steps)
{
  moveStepperSteps(steps);
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  // ----------------------------------------------------------
  // USB Serial
  // ----------------------------------------------------------

  Serial.begin(9600);

  delay(500);


  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.println(
    "     PILL DISPENSER ESP32"
  );

  Serial.println(
    "================================="
  );

  Serial.print("Firmware: ");

  Serial.println(__FILE__);


  // ----------------------------------------------------------
  // Bluetooth
  // ----------------------------------------------------------

  if (!SerialBT.begin(
        BLUETOOTH_DEVICE_NAME))
  {
    Serial.println(
      "ERROR: Bluetooth failed to start!"
    );
  }
  else
  {
    Serial.println(
      "Bluetooth started."
    );

    Serial.print(
      "Bluetooth name: "
    );

    Serial.println(
      BLUETOOTH_DEVICE_NAME
    );
  }


  // ----------------------------------------------------------
  // Stepper
  // ----------------------------------------------------------

  stepper.setMaxSpeed(1000);


  // ----------------------------------------------------------
  // ESP32 I2C
  // ----------------------------------------------------------

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  lcd.begin(
    LCD_COLUMNS,
    LCD_ROWS
  );

  lcd.backlight();

  // Register the Bluetooth icon in the LCD's custom
  // character memory (slot 0)

  lcd.createChar(
    BLUETOOTH_ICON_SLOT,
    bluetoothIcon
  );

  lcd.clear();


  lcd.setCursor(0, 0);
  lcd.print("Pill Dispenser");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");


  delay(1500);


  // ----------------------------------------------------------
  // RTC
  // ----------------------------------------------------------

  if (!rtc.begin())
  {
    Serial.println(
      "ERROR: RTC not found!"
    );


    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("RTC ERROR");

    lcd.setCursor(0, 1);
    lcd.print("Check wiring");


    while (1)
    {
      delay(1000);
    }
  }


  Serial.println(
    "RTC detected."
  );


  // ----------------------------------------------------------
  // GPIO configuration
  // ----------------------------------------------------------

  pinMode(
    LED_PIN,
    OUTPUT
  );


  pinMode(
    CONFIRM_BUTTON_PIN,
    INPUT
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  // ----------------------------------------------------------
  // Initial states
  // ----------------------------------------------------------

  digitalWrite(
    LED_PIN,
    LOW
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ----------------------------------------------------------
  // Load schedules
  // ----------------------------------------------------------

  loadSchedules();


  // ----------------------------------------------------------
  // Display stored schedules
  // ----------------------------------------------------------

  printSchedulesToSerial();


  // ----------------------------------------------------------
  // Startup complete
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  lcd.setCursor(0, 1);

  lcd.print("Schedules:");

  lcd.print(scheduleCount);


  delay(1500);


  lcd.clear();


  Serial.println(
    "System ready."
  );

  Serial.println();

  Serial.println(
    "Bluetooth commands:"
  );

  Serial.println(
    "SET,day,hour,min,sec,..."
  );

  Serial.println(
    "GET"
  );

  Serial.println(
    "CLEAR"
  );

  Serial.println(
    "<n  (jog left n steps)"
  );

  Serial.println(
    ">n  (jog right n steps)"
  );

  Serial.println(
    "TIME,year,month,day,hour,min,sec"
  );

  Serial.println();

  Serial.println(
    "Press 'u' in Serial Monitor to"
  );

  Serial.println(
    "manually update the RTC."
  );
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // Process Bluetooth
  // ----------------------------------------------------------

  processBluetooth();


  // ----------------------------------------------------------
  // Get current RTC time
  // ----------------------------------------------------------

  DateTime rtcTime =
    rtc.now();


  // ----------------------------------------------------------
  // Read confirm button
  // ----------------------------------------------------------

  buttonState =
    digitalRead(
      CONFIRM_BUTTON_PIN
    );


  // ----------------------------------------------------------
  // Update LCD
  // ----------------------------------------------------------

  updateLCD();


  // ----------------------------------------------------------
  // Check dispensing schedules
  // ----------------------------------------------------------

  checkDispensingSchedules();


  // ----------------------------------------------------------
  // CONFIRM PILL TAKEN
  // ----------------------------------------------------------

  if (buttonState == LOW)
  {
    lcd.clear();


    digitalWrite(
      LED_PIN,
      LOW
    );


    lcd.setCursor(0, 0);
    lcd.print("Pills Taken");


    delay(500);
  }


  // ----------------------------------------------------------
  // SERIAL RTC UPDATE
  // ----------------------------------------------------------

  if (Serial.available())
  {
    char input =
      Serial.read();


    if (input == 'u')
    {
      updateRTC();
    }
  }


  // Small delay prevents the loop
  // from running unnecessarily fast

  delay(20);
}
