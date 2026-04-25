int led = 9;
int brightness = 0;
int brightnessvalue = 5;
int led2 = 11;
void setup() {
  // put your setup code here, to run once:
 pinMode (led, OUTPUT);
 pinMode (led2, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
analogWrite (led, brightness);
analogWrite (led2, 255);
brightness = brightness + brightnessvalue;

if(brightness == 0 ,brightness == 255)
{
  brightnessvalue = - brightnessvalue;
  
 }
delay(50);

analogWrite (led, brightness);
analogWrite (led2, 255);
brightness = brightness + brightnessvalue;
if(brightness == 0 ,brightness == 255)
{
  brightnessvalue = - brightnessvalue;
  
 }
delay(50);

}
