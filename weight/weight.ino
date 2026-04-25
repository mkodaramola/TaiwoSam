#include <HX711.h>
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x27,16,2);

// LCD Address = 0x27



const int dtPin = 4;    // HX711 amplifier DT pin
const int sckPin = 5;   // HX711 amplifier SCK pin

const float calibrationFactor = -0.0;  // Calibration factor for the load cell

HX711 scale;

void setup() {
  
  
  
  // Initialize serial communication
    Serial.begin(9600);

  //LCD SetUp
  lcd.init();      // initialize the lcd 
  lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();

  
  // Initialize the HX711 amplifier
  scale.begin(dtPin, sckPin);
  scale.set_gain(128);
  scale.tare();

  // Calibrate the amplifier
  Serial.println("Place the load cells on a known weight and press any key to calibrate...");
  while (!Serial.available()) {
    // Wait for input from the serial monitor
  }
  scale.set_scale(-8.22);
  Serial.println("Calibration completed!");
  Serial.println();
}

void loop() {
  // Read the weight from the load cells
  float weight = scale.get_units();

  // Print the weight value
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" g");
lcd.setCursor(0,0);
lcd.print("Left weight Inbalance");
lcd.setCursor(0,1);
lcd.print("Weight:"); lcd.print(weight/1000); lcd.print("kg");

  // Delay for a short period before the next reading
  delay(2000);
  // lcd.clear();

}
