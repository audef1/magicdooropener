// Pin definitions
const int bellSensor = 0;
const int door0pener = 12;

// Tuning constants
const int rejectValue = 50;         // If an individual ring is off by this percentage of a ring we don't unlock..
const int averageRejectValue = 45;  // If the average timing of the rings is off by this percent we don't unlock.
const int ringFadeTime = 400;      // Milliseconds we allow a ring to fade before we listen for another one. (Debounce timer.)

const int maximumRings = 20;        // Maximum number of rings to listen for.
const int ringingComplete = 1500;   // Longest time to wait for a ring before we assume that it's finished.
const float aRing = 500;            // Amplitude threshold of bell ringing.

// Variables
float vin = 0.0;                    // Final voltage value of ringing bell.
int bellReadings[maximumRings];     // When someone rings this array fills with delays between rings.

// Secret ringcode
int secretCode[maximumRings] = {100, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Ring 4 times to open door.

void setup() {
  pinMode(bellSensor, INPUT);
  pinMode(door0pener, OUTPUT);

  Serial.begin(9600);
  Serial.println("Listening for bell...");
}

void loop() {

  vin = readRing();
  
  if (vin >= aRing){
    listenToSecretRing();
  }
  
}

// Reads the microphone output and converts the analog signal to a float (0.0 - 1024.0)
float readRing(){
  vin = analogRead(bellSensor);

  vin = vin * 2;
  vin = round(vin);
  vin = vin/2;

  return (vin);
}

// Records the timing of rings
void listenToSecretRing(){
  Serial.println("ringing starts");

  // First lets reset the listening array
  int i = 0;
  for (i=0;i<maximumRings;i++){
    bellReadings[i] = 0;
  }
  
  int currentRingNumber = 0;
  int startTime = millis();
  int now = millis();
  
  delay(ringFadeTime);   

  do {
    //listen for the next ring or wait for it to timeout.
    vin = readRing();
  
    if (vin >= aRing){
      //record the delay time.
      Serial.println("Ring!");
      now = millis();
      bellReadings[currentRingNumber] = now - startTime;

      Serial.print("Current Ring Number: ");
      Serial.println(currentRingNumber);
      Serial.print("delay: ");
      Serial.println(now - startTime);
      
      currentRingNumber++;
      startTime = now;

      // Add a delay and reset our timer for the next ring
      delay(ringFadeTime);
    }

    now = millis();
    
    // check for timeout or maximum rings reached
  } while ((now - startTime < ringingComplete) && (currentRingNumber < maximumRings));
  
  // check ringcode for validity
  if (validateRing() == true){
      triggerDoorUnlock(); 
  }

  Serial.println("Clearing rings...");
  
}

// Unlock the door
void triggerDoorUnlock(){
  Serial.println("Door unlocked!");
  digitalWrite(door0pener, HIGH);
  delay(1000);
  digitalWrite(door0pener, LOW);
}

// Returns true if secret ringcode was right, false if it's not.
boolean validateRing(){
  int i=0;
 
  // Check if correct amount of rings
  int currentRingCount = 0;
  int secretRingCount = 0;
  int maxRingInterval = 0;
  
  for (i = 0; i < maximumRings; i++){
    if (bellReadings[i] > 0){
      currentRingCount++;
    }
    if (secretCode[i] > 0){
      secretRingCount++;
    }
    
    // collect normalization data while we're looping.
    if (bellReadings[i] > maxRingInterval){
      maxRingInterval = bellReadings[i];
    }
  }
  
  if (currentRingCount != secretRingCount){
    return false; 
  }
  
  /*  Compare the relative intervals of our rings, not the absolute time between them.
      If you do the same pattern slow or fast it should still open the door.
  */
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<maximumRings;i++){ // Normalize the times
    bellReadings[i]= map(bellReadings[i],0, maxRingInterval, 0, 100);      
    timeDiff = abs(bellReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){ // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences/secretRingCount>averageRejectValue){
    return false; 
  }
  
  return true;
  
}

