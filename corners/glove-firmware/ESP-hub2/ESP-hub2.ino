#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// --- Hotspot config ---
const char* AP_SSID     = "ESP32-Hub";
const char* AP_PASSWORD = "password123";
const int   UDP_PORT    = 4210;

// --- Servo config ---
const int SERVO_PIN = 18; // Connect MG996R signal wire here
Servo myServo;

float distance = 0;
const int arm1 = 110;
const float arm2 = 102.5;



WiFiUDP udp;
char buf[256];

void setup() {
  Serial.begin(115200);

  // Attach the servo
  myServo.setPeriodHertz(50); // Standard 50hz servo
  myServo.attach(SERVO_PIN, 600, 2300); 

  // --- ADDED: Move to 90 degrees at startup ---
  myServo.write(90);
  delay(500); // Give the servo half a second to reach the position
  // --------------------------------------------

  WiFi.softAP(AP_SSID, AP_PASSWORD, 6, 0, 4);

  Serial.print("Hotspot started. WROOM IP: ");
  Serial.println(WiFi.softAPIP());

  udp.begin(UDP_PORT);
  Serial.println("Listening on UDP port 4210...");
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(buf, sizeof(buf) - 1);
    if (len > 0) {
      buf[len] = '\0'; // Null-terminate
      
      float valX = 0.0;
      float valZ = 0.0;
      
      // Parse the incoming comma-separated string
      if (sscanf(buf, "%f,%f", &valX, &valZ) == 2) {
        
        // 1. Constrain X to your limits (-90 to 90)
        valX = constrain(valX, -90.0, 90.0);

        valZ = constrain(valZ, 5.0, 85.0);
        
        // 2. Map the -90 to 90 range to the 0 to 180 servo range
        int servoAngleX = (int)(valX + 90.0);

        //calculates distance (bottom peice of triangle)
        distance = (valZ/85) * (arm1 + arm2);

        // cos math to calcuate the imaginary triangle. 
        int angleAtElbow = acos((arm1*arm1 + arm2*arm2 - distance*distance) / (2 * arm1 * arm2)) * 180.0 / M_PI;
        int angleAtShoulder = acos((arm1*arm1 + distance*distance - arm2*arm2) / (2 * arm1 * distance)) * 180.0 / M_PI;
        int angleAtWrist = 180.0 - angleAtShoulder - angleAtElbow;

        int servoAngleTwo = 
        int servoAngleThree = 
        int servoAngleFour = 
        
        // 3. Move the servo
        myServo.write(servoAngleX);

        // Optional: Print to serial monitor to verify
        Serial.print("Raw X: ");
        Serial.print(valX, 1);
        Serial.print(" \tMapped X Angle: ");
        Serial.println(servoAngleX);
        
      } else {
        Serial.print("Raw data received: ");
        Serial.println(buf);
      }
    }
  }
}