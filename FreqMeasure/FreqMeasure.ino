#include <FreqMeasure.h>

void setup() {
  Serial.begin(57600);
  FreqMeasure.begin();

}

unsigned long sum=0;
int count=0;

void loop() {
  Serial.println(FreqMeasure.available());
  if (FreqMeasure.available()) {
    // average several reading together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > 30) {
      unsigned long frequency = FreqMeasure.countToFrequency(sum / count);
      Serial.println(frequency);
      sum = 0;
      count = 0;
    }
  }
}
