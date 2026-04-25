#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Larger heart bitmap (20x20 pixels)
#define LOGO_HEIGHT   20
#define LOGO_WIDTH    20
static const unsigned char PROGMEM heart_bmp[] =
{ 0b00001110, 0b00001110, 0b00000000,
  0b00011111, 0b00011111, 0b10000000,
  0b00111111, 0b10111111, 0b11000000,
  0b01111111, 0b11111111, 0b11100000,
  0b01111111, 0b11111111, 0b11100000,
  0b11111111, 0b11111111, 0b11110000,
  0b11111111, 0b11111111, 0b11110000,
  0b11111111, 0b11111111, 0b11110000,
  0b11111111, 0b11111111, 0b11110000,
  0b11111111, 0b11111111, 0b11110000,
  0b01111111, 0b11111111, 0b11100000,
  0b01111111, 0b11111111, 0b11100000,
  0b00111111, 0b11111111, 0b11000000,
  0b00011111, 0b11111111, 0b10000000,
  0b00001111, 0b11111111, 0b00000000,
  0b00000111, 0b11111000, 0b00000000,
  0b00000011, 0b11110000, 0b00000000,
  0b00000001, 0b11100000, 0b00000000,
  0b00000000, 0b11000000, 0b00000000,
  0b00000000, 0b10000000, 0b00000000 };

// Data wire is connected to the Arduino digital pin 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Define the pin connections for ECG
const int ecgPin = A0;       // Output pin from AD8232 module
const int loPlusPin = 10;    // LO+ pin from AD8232 module
const int loMinusPin = 11;   // LO- pin from AD8232 module

// Button pin
const int buttonPin = 0;

// Button state
bool buttonState = HIGH;        // The current reading from the input pin
bool lastButtonState = HIGH;    // The previous reading from the input pin

// Button debounce variables
unsigned long lastDebounceTimeButton = 0;  // The last time the output pin was toggled
const unsigned long debounceDelayButton = 50;  // The debounce time; increase if the output flickers

// Debounce variables
bool lastLoPlusState = LOW;
bool lastLoMinusState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50 ms debounce delay

// Graph variables for ECG
int graphIndex = 0;
const int graphWidth = SCREEN_WIDTH;
int ecgValues[graphWidth];

// Mode selection
bool showTemperature = true;
bool ecgMessageDisplayed = false;  // New variable to track ECG mode message

void setup() {
  // Initialize serial communication for debugging purposes
  Serial.begin(9600);
  
  // Initialize the temperature sensor
  sensors.begin();
  
  // Initialize the pins for ECG
  pinMode(ecgPin, INPUT);
  pinMode(loPlusPin, INPUT);
  pinMode(loMinusPin, INPUT);

  // Initialize the button pin with internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Clear the buffer
  display.clearDisplay();
  
  // Animate the heart moving from the left to the center
  for (int x = -LOGO_WIDTH; x <= (display.width() - LOGO_WIDTH) / 2; x++) {
    display.clearDisplay();
    display.drawBitmap(
      x, // X position (moving)
      (display.height() - LOGO_HEIGHT) / 2, // Y position (centered vertically)
      heart_bmp, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(20);
  }

  // Wait for 2 seconds
  delay(2000);

  // Clear the buffer and show initial text
  display.clearDisplay();
  testdrawroundrect(); 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, (display.height() - 8) / 2); // Adjust Y position as needed
  display.print("  Health Monitoring          Device");
  display.display();
  delay(2000);

  // Initialize the variable
  ecgMessageDisplayed = false;
}

void loop() {
  // Read the button state
  int reading = digitalRead(buttonPin);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    // Reset the debouncing timer
    lastDebounceTimeButton = millis();
  }

  if ((millis() - lastDebounceTimeButton) > debounceDelayButton) {
    // If the button state has been stable for the debounce delay, update the button state
    if (reading != buttonState) {
      buttonState = reading;
      
      // If the button was pressed
      if (buttonState == LOW) {
        showTemperature = !showTemperature;  // Toggle the display mode
        ecgMessageDisplayed = false;  // Reset ECG message display status
      }
    }
  }

  // Save the reading for next time
  lastButtonState = reading;

  // Print the button state to the Serial Monitor for debugging
  Serial.print("Button State: ");
  Serial.println(buttonState);

  // Request temperature readings from the sensor
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  // Read the state of the LO pins
  bool loPlusState = digitalRead(loPlusPin);
  bool loMinusState = digitalRead(loMinusPin);

  // Check for changes in LO state
  if ((loPlusState != lastLoPlusState) || (loMinusState != lastLoMinusState)) {
    // Reset the debouncing timer
    lastDebounceTime = millis();
  }

  // If the change in state has been stable for longer than debounceDelay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Update the stable state
    lastLoPlusState = loPlusState;
    lastLoMinusState = loMinusState;
  }

  // Clear the display
  display.clearDisplay();

  if (showTemperature) {
    // Draw the temperature reading
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(2, (display.height() - 8) / 2);
    display.print("Body Temp: ");
    display.print(temperatureC);
    display.print(" C");

    // Print temperature to Serial Monitor for debugging
    Serial.print("Body Temp: ");
    Serial.print(temperatureC);
    Serial.println(" C");

    // Ensure ECG mode message has not been shown
    ecgMessageDisplayed = false;
  } else {
    if (lastLoPlusState == LOW && lastLoMinusState == LOW) {
      if (!ecgMessageDisplayed) {
        // Show the ECG mode activation message once
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(2, (display.height() - 8) / 2);
        display.print("ECG Mode Activated");
        display.display();
        delay(2000);
        display.clearDisplay();
        
        ecgMessageDisplayed = true;  // Mark the message as displayed
      }
      
      // Draw the ECG graph
      int ecgValue = analogRead(ecgPin);
      ecgValues[graphIndex] = map(ecgValue, 0, 1023, SCREEN_HEIGHT, 0);
    
      for (int i = 0; i < graphWidth - 1; i++) {
        display.drawLine(i, ecgValues[i], i + 1, ecgValues[i + 1], SSD1306_WHITE);
      }
      
      graphIndex++;
      if (graphIndex >= graphWidth) {
        graphIndex = 0;
      }
    } else {
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(2, (display.height() - 8) / 2 + 10);
      display.print("Electrodes not connected properly.");
    }
  }

  // Display the buffer on the screen
  display.display();

  // Delay before repeating the loop
  delay(1);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i,
      display.height() / 4, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}
