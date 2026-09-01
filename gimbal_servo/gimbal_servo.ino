#include <Arduino_LSM9DS1.h>
#include <Servo.h>

Servo gimbalServo;

const int servoPin = A7;

/* -------- PID PARAMETERS -------- */
const float targetAngleDeg = 4.0;

float Kp = 1.1;
float Ki = 0.01;
float Kd = 0.008;

/* -------- COMPLEMENTARY FILTER -------- */
const float alpha = 0.98;

/* -------- DEBUG -------- */
const bool enableSerialDebug = false;
const unsigned long printIntervalMs = 100;

/* -------- SENSOR DATA -------- */
float ax, ay, az;
float gx, gy, gz;

/* -------- ANGLE VARIABLES -------- */
float filteredAngleDeg = 0;

/* -------- PID VARIABLES -------- */
float angleError = 0;
float prevError = 0;
float integral = 0;
float derivative = 0;
float pidOutput = 0;

/* -------- SERVO -------- */
float servoAngle = 88;

/* -------- TIMING -------- */
unsigned long prevMicros = 0;
unsigned long lastServoUpdate = 0;
unsigned long lastPrintTime = 0;

const int servoUpdateInterval = 10;

void setup()
{
    if (enableSerialDebug) Serial.begin(115200);

    if (!IMU.begin())
    {
        if (enableSerialDebug) Serial.println("IMU failed");
        while (1);
    }

    gimbalServo.attach(servoPin);
    gimbalServo.write(90);

    delay(200);

    while (!IMU.accelerationAvailable());
    IMU.readAcceleration(ax, ay, az);

    filteredAngleDeg = atan2(ay, az) * 180.0 / PI;

    prevMicros = micros();

    if (enableSerialDebug) Serial.println("System ready");
}

void loop()
{
    updateAngle();
    runPID();
    updateServo();
    printStatus();
}

/* -------- ANGLE ESTIMATION -------- */

void updateAngle()
{
    if (IMU.accelerationAvailable())
        IMU.readAcceleration(ax, ay, az);

    if (IMU.gyroscopeAvailable())
        IMU.readGyroscope(gx, gy, gz);

    unsigned long now = micros();
    float dt = (now - prevMicros) / 1000000.0;
    prevMicros = now;

    if (dt <= 0 || dt > 0.05) return;

    float accelAngle = atan2(ay, az) * 180.0 / PI;

    filteredAngleDeg =
        alpha * (filteredAngleDeg + gx * dt) +
        (1.0 - alpha) * accelAngle;

    if (filteredAngleDeg > 90) filteredAngleDeg = 90;
    if (filteredAngleDeg < -90) filteredAngleDeg = -90;
}

/* -------- PID CONTROL -------- */

void runPID()
{
    angleError = targetAngleDeg - filteredAngleDeg;

    if (abs(angleError) < 0.3) return;

    float dt = servoUpdateInterval / 1000.0;

    integral += angleError * dt;
    derivative = (angleError - prevError) / dt;

    pidOutput =
        (Kp * angleError) +
        (Ki * integral) +
        (Kd * derivative);

    prevError = angleError;

    servoAngle = 90 - pidOutput;

    if (servoAngle > 180) servoAngle = 180;
    if (servoAngle < 0) servoAngle = 0;
}

/* -------- SERVO UPDATE -------- */

void updateServo()
{
    if (millis() - lastServoUpdate >= servoUpdateInterval)
    {
        gimbalServo.write(servoAngle);
        lastServoUpdate = millis();
    }
}

/* -------- SERIAL DEBUG -------- */

void printStatus()
{
    if (!enableSerialDebug) return;

    unsigned long now = millis();

    if (now - lastPrintTime < printIntervalMs)
        return;

    lastPrintTime = now;

    Serial.print("Angle: ");
    Serial.print(filteredAngleDeg, 2);

    Serial.print("  Servo: ");
    Serial.println(servoAngle, 2);
}