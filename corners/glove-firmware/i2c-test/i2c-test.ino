#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\nI2C Scanner Started");

  // Explicitly set pins for the C3 Super Mini
  Wire.begin(8, 9); 
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for(address = 1; address < 127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("SUCCESS: I2C device found at address 0x");
      if (address < 16) 
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("!");
      nDevices++;
    }
    
  }
  
  if (nDevices == 0) {
    Serial.println("ERROR: No I2C devices found. The board cannot see the sensor.");
  } else {
    Serial.println("Scan complete.\n");
  }

  delay(5000); // Wait 5 seconds for next scan
}
