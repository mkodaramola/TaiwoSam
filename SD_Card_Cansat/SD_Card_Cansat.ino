#include <SPI.h>
#include <SD.h>

File myFile;

int i = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

i =  M_read("rd.txt",2).toInt();




 

 
}


void loop() {
  String con = "12:05:47,"+ String(i) +",333";
  M_write("rd.txt",con, false);

delay(1000);
  Serial.print("Content: ");
  Serial.println(M_read("rd.txt",4));
  Serial.print("Time: ");
  Serial.println(M_read("rd.txt",1));
  Serial.print("Packet count: ");
  Serial.println(M_read("rd.txt",2));
  Serial.print("Cal Value: ");
  Serial.println(M_read("rd.txt",3));
    i++;

delay(1000);
  
  

}





void M_write(String fname, String content, boolean append){

  myFile = SD.open(fname, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {

     if (!append){

      myFile = SD.open(fname, O_WRITE | O_TRUNC);
         
    }
    
    myFile.println(content);
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

   
  }

String M_read(String fname, byte n){
  String text = "";
  myFile = SD.open(fname);
  if (myFile) {

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      text += (char)myFile.read();
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  byte fc = text.indexOf(",");
  byte lc = text.lastIndexOf(",");
  

  if (n == 1) text = text.substring(0,fc);
  else if (n == 2) text = text.substring(fc+1,lc);
  else if (n == 3) text = text.substring(lc+1);
 
    
  return text; 
  
  }
