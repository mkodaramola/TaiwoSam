#include <Wire.h>
#include <RTClib.h>

// Create an RTC object
RTC_DS3231 rtc;

void setup() {
  // Start the Serial communication
  Serial.begin(9600);

  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Check if the RTC lost power and if so, set the time
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    // This line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // You can also set the RTC with an explicit date & time, e.g.:
    rtc.adjust(DateTime(2023, 7, 16, 12, 0, 0));
  }
}

void loop() {
  // Get the current time
  DateTime now = rtc.now();

  // Print the time in HH:MM:SS format
  Serial.print(now.hour());
  Serial.print(':');
  Serial.print(now.minute());
  Serial.print(':');
  Serial.println(now.second());

  // Wait for 1 second before repeating
  delay(1000);
  Serial.println("pppp");
}
