/**
 * Reads X/Y values from a PS/2 mouse connected to an Arduino
 * using the PS2Mouse library available from
 *   http://github.com/kristopher/PS2-Mouse-Arduino/
 * Original by Kristopher Chambers <kristopher.chambers@gmail.com>
 * Updated by Jonathan Oxer <jon@oxer.com.au>
 */

#include <PS2Mouse.h>
#define MOUSE_DATA 5
#define MOUSE_CLOCK 6

PS2Mouse mouse(MOUSE_CLOCK, MOUSE_DATA, STREAM);


int x =0;
int y =0;


/**
 * Setup
 */
void setup()
{

  Serial.begin(38400);
  
  Serial.println("Init start.....");

  delay(1000);  // after power-on before `mouse.initialise();`

  mouse.initialize();

  Serial.println("Done.....");
}

/**
 * Main program loop
 */
 
unsigned long last_run = millis();
void loop()
{
  int16_t data[3];
  mouse.report(data);
  
   if(data[1] > 0) x+=1;
   else if(data[1] < 0) x-=1;

   if(data[2] > 0) y+=1;
   else if(data[2] < 0) y-=1;

   if (millis() - last_run > 200) {
    last_run = millis();
  Serial.print(data[0]); // Status Byte
  Serial.print(":");
  Serial.print(x); // X Movement Data
  Serial.print(",");
  Serial.print(y); // Y Movement Data
  Serial.println();
   }




}
