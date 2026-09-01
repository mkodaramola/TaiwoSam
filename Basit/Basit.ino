/*
   Final Pill Dispenser
   ESP32 DevKit V1 - 30 Pin

   Original algorithm retained from FinalPillDispenser.ino.

   Hardware:
   - ESP32 DevKit V1
   - DS3231 RTC
   - 16x2 I2C LCD
   - 28BYJ-48 Stepper Motor
   - ULN2003 Driver
   - Confirm Button
   - Buzzer
   - LED

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
   NOTE: GPIO2/LED is retained from the original code.
   The supplied schematic does not explicitly show an LED
   connection to GPIO2.

   ============================================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <AccelStepper.h>


// ============================================================
// LCD
// ============================================================

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS    2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);


// ============================================================
// RTC
// ============================================================

RTC_DS3231 rtc;


// ============================================================
// ESP32 PIN DEFINITIONS
// ============================================================

// -------------------- ULN2003 / 28BYJ-48 STEPPER --------------------

// Schematic:
// GPIO26 -> IN1
// GPIO27 -> IN2
// GPIO14 -> IN3
// GPIO13 -> IN4

#define motorPin1  26
#define motorPin2  27
#define motorPin3  14
#define motorPin4  13


// AccelStepper interface type
// 8 = 4-wire motor in half-step mode
// 4096 steps approximately = one output-shaft revolution

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

// Schematic:
// GPIO21 -> SDA
// GPIO22 -> SCL

#define I2C_SDA 21
#define I2C_SCL 22


// ============================================================
// CONFIRM BUTTON
// ============================================================

// Schematic:
// GPIO4 -> conf_btn
//
// The schematic has a 10k pull-up and the button connects
// conf_btn to GND.
//
// Therefore:
// Button NOT pressed = HIGH
// Button PRESSED     = LOW

#define CONFIRM_BUTTON_PIN 4


// ============================================================
// BUZZER
// ============================================================

// GPIO25 controls the buzzer transistor circuit.
// GPIO25 does NOT directly drive the buzzer.

#define BUZZER_PIN 25


// ============================================================
// LED
// ============================================================

// GPIO2 is retained from the original program.
// The supplied schematic does not explicitly show the LED
// connection to GPIO2.

#define LED_PIN 2


// ============================================================
// DISPENSING TIME SETTINGS
// ============================================================

// AM dispensing time
int amHr  = 8;
int amMin = 0;
int amSec = 0;


// PM dispensing time
int pmHr  = 21;
int pmMin = 0;
int pmSec = 0;


// ============================================================
// VARIABLES
// ============================================================

int buttonState = HIGH;


// ============================================================
// INPUT VALIDATION STRUCTURE
// ============================================================

typedef struct minMax_t {
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


  // Prompts for user input

  const char txt[6][15] =
  {
    "year [4-digit]",
    "month [1~12]",
    "day [1~31]",
    "hours [0~23]",
    "minutes [0~59]",
    "seconds [0~59]"
  };


  // Minimum and maximum values

  const minMax_t minMax[] =
  {
    {2000, 9999},   // Year
    {1, 12},        // Month
    {1, 31},        // Day
    {0, 23},        // Hours
    {0, 59},        // Minutes
    {0, 59}         // Seconds
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


        // ----------------------------------------------------
        // Abort
        // ----------------------------------------------------

        if ((str == "-1")   ||
            (str == "-1\n") ||
            (str == "-1\r") ||
            (str == "-1\r\n"))
        {
          Serial.println("\nABORTED");

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
}


// ============================================================
// UPDATE LCD
// ============================================================

void updateLCD()
{
  // Get current time

  DateTime rtcTime = rtc.now();


  // Time format

  char timeBuffer[] = "hh:mm:ss";


  // ----------------------------------------------------------
  // Display time
  // ----------------------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("Time");


  lcd.setCursor(5, 0);
  lcd.print(rtcTime.toString(timeBuffer));
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  // ----------------------------------------------------------
  // Serial
  // ----------------------------------------------------------

  Serial.begin(9600);

  Serial.println();
  Serial.println("=================================");
  Serial.println("     PILL DISPENSER ESP32");
  Serial.println("=================================");
  Serial.print("Firmware: ");
  Serial.println(__FILE__);


  // ----------------------------------------------------------
  // Stepper
  // ----------------------------------------------------------

  stepper.setMaxSpeed(1000);


  // ----------------------------------------------------------
  // ESP32 I2C
  // ----------------------------------------------------------

  // Schematic:
  // GPIO21 = SDA
  // GPIO22 = SCL

  Wire.begin(I2C_SDA, I2C_SCL);


  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  lcd.begin();

  lcd.backlight();

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
    Serial.println("ERROR: RTC not found!");

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


  Serial.println("RTC detected.");


  // ----------------------------------------------------------
  // GPIO configuration
  // ----------------------------------------------------------

  pinMode(LED_PIN, OUTPUT);

  pinMode(CONFIRM_BUTTON_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);


  // Initial states

  digitalWrite(LED_PIN, LOW);

  digitalWrite(BUZZER_PIN, LOW);


  // ----------------------------------------------------------
  // Startup complete
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  delay(1000);

  lcd.clear();


  Serial.println("System ready.");
  Serial.println();
  Serial.println("Press 'u' in Serial Monitor to");
  Serial.println("manually update the RTC.");
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // Get current RTC time
  // ----------------------------------------------------------

  DateTime rtcTime = rtc.now();


  // ----------------------------------------------------------
  // Read confirm button
  // ----------------------------------------------------------

  buttonState = digitalRead(CONFIRM_BUTTON_PIN);


  // ----------------------------------------------------------
  // Update LCD
  // ----------------------------------------------------------

  updateLCD();


  // ==========================================================
  // AM DISPENSING
  // ==========================================================

  if (rtcTime.hour() == amHr &&
      rtcTime.minute() == amMin &&
      rtcTime.second() == amSec)
  {
    lcd.clear();

    rotateAM();
  }


  // ==========================================================
  // PM DISPENSING
  // ==========================================================

  if (rtcTime.hour() == pmHr &&
      rtcTime.minute() == pmMin &&
      rtcTime.second() == pmSec)
  {
    lcd.clear();

    rotatePM();
  }


  // ==========================================================
  // CONFIRM PILL TAKEN
  // ==========================================================

  // Schematic button wiring is active LOW:
  //
  // Not pressed = HIGH
  // Pressed     = LOW

  if (buttonState == LOW)
  {
    lcd.clear();

    digitalWrite(LED_PIN, LOW);

    lcd.setCursor(0, 1);

    lcd.print("Pills Taken");


    // Original algorithm had this commented out.
    // rotateOver();
  }


  // ==========================================================
  // SERIAL RTC UPDATE
  // ==========================================================

  if (Serial.available())
  {
    char input = Serial.read();

    if (input == 'u')
    {
      updateRTC();
    }
  }
}


// ============================================================
// AM PILL DISPENSING
// ============================================================

void rotateAM()
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
  //
  // 28BYJ-48:
  // Approximate gear ratio = 7.5:1
  //
  // 4096 half steps ≈ one output-shaft revolution
  //
  // 2048 steps ≈ half revolution
  //
  // ----------------------------------------------------------

  while (stepper.currentPosition() != 2048)
  {
    stepper.setSpeed(500);

    stepper.runSpeed();
  }


  // ----------------------------------------------------------
  // Disable motor outputs
  // ----------------------------------------------------------

  stepper.disableOutputs();


  // ----------------------------------------------------------
  // Indicate pills are ready
  // ----------------------------------------------------------

  digitalWrite(LED_PIN, HIGH);


  lcd.setCursor(0, 1);

  lcd.print("AM Pills Ready");
}


// ============================================================
// PM PILL DISPENSING
// ============================================================

void rotatePM()
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

  while (stepper.currentPosition() != 2048)
  {
    stepper.setSpeed(500);

    stepper.runSpeed();
  }


  // ----------------------------------------------------------
  // Disable motor outputs
  // ----------------------------------------------------------

  stepper.disableOutputs();


  // ----------------------------------------------------------
  // Indicate pills are ready
  // ----------------------------------------------------------

  lcd.setCursor(0, 1);

  lcd.print("PM Pills Ready");

  digitalWrite(LED_PIN, HIGH);
}


// ============================================================
// OVERRIDE TIMED ROTATION
// ============================================================

void rotateOver()
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

  while (stepper.currentPosition() != 2048)
  {
    stepper.setSpeed(500);

    stepper.runSpeed();
  }


  // ----------------------------------------------------------
  // Disable motor outputs
  // ----------------------------------------------------------

  stepper.disableOutputs();


  // ----------------------------------------------------------
  // Display override message
  // ----------------------------------------------------------

  lcd.setCursor(0, 1);

  lcd.print("Timer Overridden");

  digitalWrite(LED_PIN, HIGH);
}