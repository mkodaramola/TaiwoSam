const float PWM_FREQ_KHZ = 200.0;

void setupPWM(float freq_khz)
{
    pinMode(9, OUTPUT);

    float freq = freq_khz * 1000.0;

    // Clear timer registers
    TCCR1A = 0;
    TCCR1B = 0;

    // Fast PWM mode with ICR1 as TOP
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);

    // Non-inverting PWM on OC1A (D9)
    TCCR1A |= (1 << COM1A1);

    // No prescaler
    TCCR1B |= (1 << CS10);

    // Set TOP for frequency
    ICR1 = (16000000.0 / freq) - 1;
}

void setDutyCycle(float duty_percent)
{
    // Limit from 0 to 50%
    if(duty_percent < 0) duty_percent = 0;
    if(duty_percent > 50) duty_percent = 50;

    OCR1A = ((ICR1 + 1) * duty_percent) / 100.0;
}

void setup()
{
    setupPWM(PWM_FREQ_KHZ);
}

void loop()
{
    // Read potentiometer
    int potValue = analogRead(A0);

    // Convert 0-1023 to 0-50%
    float duty = (potValue / 1023.0) * 50.0;

    // Update PWM duty cycle
    setDutyCycle(duty);

    delay(5);
}