int i = 0;
void setup()
{
   // Start hardware serial communication with the XBee module
  Serial1.begin(9600);  // Ensure this matches the XBee baud rate

  
}

void loop()
{ 
  String t = "Data " + String(i); 
  
  Serial1.println(t);  // Send 'H' to the XBee module
  
  delay(1000); 



  
  i++;
 }
