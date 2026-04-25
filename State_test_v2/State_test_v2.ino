// Define the pins for altitude sensor and any other necessary variables
const int altitudeSensorPin = A0; // Analog pin for altitude sensor
int currentAltitude; 
int previousAltitude;
int ascentThreshold = 3; 
int descentThreshold = -3; 

void setup() {
  Serial.begin(9600); // Initialize serial communication
}

void loop() {
  currentAltitude = analogRead(altitudeSensorPin);
  CSstate(currentAltitude);
 
}


void CSstate(float currentAltitude){
  
  // Check if the previous altitude is available (not the first loop iteration)
  if (previousAltitude != 0) {
    int altitudeChange = currentAltitude - previousAltitude;
    Serial.print("Alt: ");
    Serial.println(currentAltitude);
    Serial.print("Alt Change:");
    Serial.println(altitudeChange);
    // Determine the altitude trend
    
    if (altitudeChange > ascentThreshold) {
      Serial.println("Ascent");
    } else if (altitudeChange < descentThreshold) {
      Serial.println("Descent");
    } else if(altitudeChange < descentThreshold && currentAltitude <-100){
    Serial.println("HEAT SHIED DEPLOYED");
    }
  } else {
    // First loop iteration, set previous altitude
    previousAltitude = currentAltitude;
  }

  // Update previous altitude for the next iteration
  previousAltitude = currentAltitude;
  
  // Add a delay to control the frequency of altitude readings
  delay(1000); 
}