#include "Arduino.h"
#include <HX711.h>

void calibrate();

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;


HX711 scale;

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 Demo");

  Serial.println("Initializing the scale");

  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_gain(128);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
  // by the SCALE parameter (not set yet)

  scale.set_scale(-7.98);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0

  Serial.println("After setting up the scale:");

//  calibrate();

//  Serial.print("read: \t\t");
//  Serial.println(scale.read());                 // print a raw reading from the ADC
//
//  Serial.print("read average: \t\t");
//  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC
//
//  Serial.print("get value: \t\t");
//  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()
//
//  Serial.print("get units: \t\t");
//  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
//  // by the SCALE parameter set with set_scale
//
//  Serial.println("Readings:");
}

void loop() {
  Serial.print("one reading:\t");
  Serial.print(scale.get_units(), 1);
  Serial.print("\t| average:\t");
  Serial.println(scale.get_units(10), 1);

//  scale.power_down();             // put the ADC in sleep mode
  delay(2000);
//  scale.power_up();
}

void calibrate() {
  // Remove any calibration values and clear the scale
  scale.set_scale();
  scale.tare();

  // Prompt the user
  Serial.println("Add your known weight to the scale, enter the weight and press <Enter>");
  int userInput = -123;
  String inputString = "";
  // Loop until we receive an input (which will change the value of userInput
  while (userInput == -123) {
    // Read serial input:
    while (Serial.available() > 0) {
      int inputChar = Serial.read();
      if (isDigit(inputChar)) {
        // convert the incoming byte to a char and add it to the string:
        inputString += (char)inputChar;
      }
      // if you get a newline, print the string, then the string's value:
      if (inputChar == '\n') {
        userInput = inputString.toInt();
      }
    }
  }

  // Now get the reading from the scale
  float calReading = scale.get_units(10);

  Serial.print("Setting the cal to ");
  Serial.println(calReading / userInput);

  scale.set_scale(calReading / userInput);
}
