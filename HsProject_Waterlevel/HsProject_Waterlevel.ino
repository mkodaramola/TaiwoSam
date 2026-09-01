#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C Address (change to 0x3F if necessary)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensorPin = A2;
const int threshold = 460;

const int buzzer = 11;
const int pump = 9;


void setup()
{
    Serial.begin(9600);

    pinMode(buzzer, OUTPUT);
    pinMode(pump, OUTPUT);
    digitalWrite(buzzer, LOW);

    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("Water Level");
    lcd.setCursor(0, 1);
    lcd.print("Monitoring...");
    delay(2000);
    lcd.clear();
}

void loop()
{
    int sensorValue = analogRead(sensorPin);

    Serial.print("Water Level: ");
    Serial.println(sensorValue);

    // Display sensor value
    lcd.setCursor(0, 0);
    lcd.print("Level: ");
    lcd.print(sensorValue);
    lcd.print("    ");   // Clear leftover digits

    if (sensorValue > threshold)
    {
        digitalWrite(buzzer, HIGH);
        digitalWrite(pump, HIGH);

        lcd.setCursor(0, 1);
        lcd.print("Tank Full!     ");

        Serial.println("Water level is HIGH!");
    }
    else
    {
        digitalWrite(buzzer, LOW);
        digitalWrite(pump, LOW);

        lcd.setCursor(0, 1);
        lcd.print("Tank Not Full  ");
    }

    delay(500);
}