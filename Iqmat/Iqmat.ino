/*
  ============================================================
       DUST EXPOSURE INDICATOR
       ESP32-C3 SuperMini + GP2Y1010AU0F
  ============================================================

  PIN CONNECTIONS
  ------------------------------------------------------------
  GP2Y1010AU0F:
      Vo          -> GPIO0
      LED control -> GPIO1

  Indicator LEDs:
      Green       -> GPIO3
      Yellow      -> GPIO4
      Red         -> GPIO5

  LED behaviour:
      Green  = Low dust
      Yellow = Moderate dust
      Red    = High dust
  ============================================================
*/


// ============================================================
// PIN DEFINITIONS
// ============================================================

// GP2Y1010AU0F
const int DUST_SENSOR_PIN = 0;     // Vo
const int DUST_LED_PIN    = 1;     // LED control

// Dust indicator LEDs
const int GREEN_LED_PIN  = 3;
const int YELLOW_LED_PIN = 4;
const int RED_LED_PIN    = 5;


// ============================================================
// GP2Y1010 SENSOR SETTINGS
// ============================================================

// GP2Y1010 timing
const unsigned int SENSOR_LED_ON_TIME = 280;   // microseconds
const unsigned int SENSOR_SAMPLE_TIME = 40;    // microseconds
const unsigned int SENSOR_CYCLE_TIME  = 9680;  // microseconds


// ============================================================
// DUST CALIBRATION
// ============================================================

// Typical clean-air output of GP2Y1010
const float CLEAN_AIR_VOLTAGE = 0.90;

// Typical sensor sensitivity
// 0.5 V corresponds to approximately 0.1 mg/m³
const float SENSOR_SENSITIVITY = 0.50;


// ============================================================
// DUST EXPOSURE THRESHOLDS
// ============================================================

// Values are in mg/m³
//
// Green:
//     < 0.15 mg/m³
//
// Yellow:
//     0.15 - 0.30 mg/m³
//
// Red:
//     >= 0.30 mg/m³

const float GREEN_THRESHOLD = 0.15;
const float RED_THRESHOLD   = 0.30;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // Start Serial Monitor
    Serial.begin(9600);

    // --------------------------------------------------------
    // Configure GP2Y1010
    // --------------------------------------------------------

    pinMode(DUST_LED_PIN, OUTPUT);

    // Sensor LED is OFF initially
    digitalWrite(DUST_LED_PIN, HIGH);


    // --------------------------------------------------------
    // Configure indicator LEDs
    // --------------------------------------------------------

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    // Turn all indicator LEDs OFF
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);


    // --------------------------------------------------------
    // Configure ESP32-C3 ADC
    // --------------------------------------------------------

    analogReadResolution(12);


    // --------------------------------------------------------
    // Give the sensor time to stabilize
    // --------------------------------------------------------

    delay(1000);


    Serial.println();
    Serial.println("==============================");
    Serial.println(" Dust Exposure Indicator");
    Serial.println("==============================");
    Serial.println("System started.");
    Serial.println();
}


// ============================================================
// READ DUST SENSOR
// ============================================================

float readDustVoltage()
{
    /*
      GP2Y1010 measurement sequence:

      1. Turn the internal sensor LED ON
      2. Wait approximately 280 us
      3. Read the sensor output
      4. Turn the LED OFF
      5. Wait for the remainder of the 10 ms cycle
    */


    // Turn sensor LED ON
    digitalWrite(DUST_LED_PIN, LOW);


    // Wait for sensor LED pulse timing
    delayMicroseconds(SENSOR_LED_ON_TIME);


    // Read analog output
    int adcValue = analogRead(DUST_SENSOR_PIN);


    // Complete the sampling period
    delayMicroseconds(SENSOR_SAMPLE_TIME);


    // Turn sensor LED OFF
    digitalWrite(DUST_LED_PIN, HIGH);


    // Complete approximately 10 ms measurement cycle
    delayMicroseconds(SENSOR_CYCLE_TIME);


    // Convert ADC reading to voltage
    //
    // ESP32-C3 ADC:
    // 12-bit = 0 to 4095
    //
    // Assuming approximately 3.3 V ADC range

    float voltage = (adcValue / 4095.0) * 3.3;


    return voltage;
}


// ============================================================
// CONVERT SENSOR VOLTAGE TO DUST DENSITY
// ============================================================

float calculateDustDensity(float voltage)
{
    float dustDensity;


    /*
      Approximate GP2Y1010 relationship:

          Dust density =
          ((Vo - clean-air voltage)
          / sensitivity) × 0.1

      Where:

          Clean-air voltage = 0.9 V

          Sensitivity = 0.5 V / 0.1 mg/m³
    */


    // If voltage is below clean-air baseline,
    // treat it as zero dust.
    if (voltage <= CLEAN_AIR_VOLTAGE)
    {
        dustDensity = 0.0;
    }
    else
    {
        dustDensity =
            ((voltage - CLEAN_AIR_VOLTAGE)
            / SENSOR_SENSITIVITY) * 0.1;
    }


    return dustDensity;
}


// ============================================================
// UPDATE INDICATOR LED
// ============================================================

void updateDustIndicator(float dustDensity)
{
    // Turn all LEDs OFF first
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);


    // --------------------------------------------------------
    // LOW DUST
    // --------------------------------------------------------

    if (dustDensity < GREEN_THRESHOLD)
    {
        digitalWrite(GREEN_LED_PIN, HIGH);
    }


    // --------------------------------------------------------
    // MODERATE DUST
    // --------------------------------------------------------

    else if (dustDensity < RED_THRESHOLD)
    {
        digitalWrite(YELLOW_LED_PIN, HIGH);
    }


    // --------------------------------------------------------
    // HIGH DUST
    // --------------------------------------------------------

    else
    {
        digitalWrite(RED_LED_PIN, HIGH);
    }
}


// ============================================================
// PRINT SENSOR DATA
// ============================================================

void printDustData(float voltage, float dustDensity)
{
    // Convert mg/m³ to µg/m³
    float dustMicrograms = dustDensity * 1000.0;


    Serial.print("Sensor Voltage: ");
    Serial.print(voltage, 3);
    Serial.print(" V");


    Serial.print("   |   Dust: ");
    Serial.print(dustDensity, 4);
    Serial.print(" mg/m3");


    Serial.print("   |   ");
    Serial.print(dustMicrograms, 1);
    Serial.print(" ug/m3");


    Serial.print("   |   Status: ");


    // Determine current status
    if (dustDensity < GREEN_THRESHOLD)
    {
        Serial.println("LOW - GREEN");
    }
    else if (dustDensity < RED_THRESHOLD)
    {
        Serial.println("MODERATE - YELLOW");
    }
    else
    {
        Serial.println("HIGH - RED");
    }
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Read GP2Y1010
    // --------------------------------------------------------

    float sensorVoltage = readDustVoltage();


    // --------------------------------------------------------
    // Calculate dust concentration
    // --------------------------------------------------------

    float dustDensity = calculateDustDensity(sensorVoltage);


    // --------------------------------------------------------
    // Update LED indicator
    // --------------------------------------------------------

    updateDustIndicator(dustDensity);


    // --------------------------------------------------------
    // Display information on Serial Monitor
    // --------------------------------------------------------

    printDustData(sensorVoltage, dustDensity);


    // --------------------------------------------------------
    // Wait before next measurement
    // --------------------------------------------------------

    delay(100);
}