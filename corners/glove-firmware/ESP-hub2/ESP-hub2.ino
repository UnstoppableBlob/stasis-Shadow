#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// --- Hotspot config ---
const char* AP_SSID     = "ESP32-Hub";
const char* AP_PASSWORD = "password123";
const int   UDP_PORT    = 4210;

// --- Servo pins ---
const int PIN_SHOULDER = 14;
const int PIN_ELBOW    = 25;
const int PIN_WRIST    = 26;
const int SERVO_PIN    = 13;

Servo servo1;
Servo servoShoulder, servoElbow, servoWrist;

// --- ARM DIMENSIONS ---
float arm1        = 110.0;
float arm2        = 102.5;
float totalLength = arm1 + arm2;

// --- CALIBRATION: tune these ---
bool  reverseElbow    = false;
bool  reverseShoulder = true;
bool  reverseWrist    = false;

float offsetElbow    = 75.0;
float offsetShoulder = 130.0;
float offsetWrist    = 0.0;

float distance = 0;
WiFiUDP udp;

struct SensorData {
  float yaw;
  float roll;
};

  float clamp(float v) { return constrain (v, -1.0, 1.0); }
// --- TRIANGLE MATH ---
void getAngles(float pitch, int &angleElbow, int &angleShoulder, int &angleWrist) {
  pitch = constrain(pitch, 5.0, 85.0);

  float dist = ((pitch - 5.0) / (85.0 - 5.0)) * totalLength;
  dist = constrain(dist, 1.0, totalLength - 1.0);



  float rawElbow    = acos(clamp((arm1*arm1 + arm2*arm2 - dist*dist)/ (2.0 * arm1 * arm2))) * 180.0 / M_PI;
  float rawShoulder = acos(clamp((arm1*arm1 + dist*dist - arm2*arm2)/ (2.0 * arm1 * dist))) * 180.0 / M_PI;
  float rawWrist    = 180.0 - rawShoulder - rawElbow;

  angleElbow    = constrain((reverseElbow    ? 180.0 - rawElbow    : rawElbow)    + offsetElbow,    0, 180);
  angleShoulder = constrain((reverseShoulder ? 180.0 - rawShoulder : rawShoulder) + offsetShoulder, 0, 180);
  angleWrist    = constrain((reverseWrist    ? 180.0 - rawWrist    : rawWrist)    + offsetWrist,    0, 180);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("BOOTING...");

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO_PIN, 600, 2300);
  servoShoulder.attach(PIN_SHOULDER);
  servoElbow.attach(PIN_ELBOW);
  servoWrist.attach(PIN_WRIST);

  // Initialize to pitch=5 (all the way in)
  int initElbow, initShoulder, initWrist;
  getAngles(5.0, initElbow, initShoulder, initWrist);

  servo1.write(90);
  servoElbow.write(initElbow);
  servoShoulder.write(initShoulder);
  servoWrist.write(initWrist);

  Serial.print("Init — Elbow: ");    Serial.print(initElbow);
  Serial.print("  Shoulder: ");      Serial.print(initShoulder);
  Serial.print("  Wrist: ");         Serial.println(initWrist);

  delay(500);

  WiFi.softAP(AP_SSID, AP_PASSWORD, 6, 0, 4);
  Serial.print("Hotspot started. IP: ");
  Serial.println(WiFi.softAPIP());
  udp.begin(UDP_PORT);
  Serial.println("Listening on UDP port 4210...");
}

void loop() {
  int packetSize = udp.parsePacket();

  if (packetSize == sizeof(SensorData)) {
    SensorData receivedData;
    udp.read((char*)&receivedData, sizeof(receivedData));

    float valX = receivedData.yaw;
    float valZ = receivedData.roll;

    valX = constrain(valX, -90.0, 90.0);
    valZ = constrain(valZ,   5.0, 85.0);

    // Yaw → servo1
    int servoAngleX = (int)(valX + 90.0);
    servo1.write(servoAngleX);

    // Pitch → triangle servos
    int angleElbow, angleShoulder, angleWrist;
    getAngles(valZ, angleElbow, angleShoulder, angleWrist);

    servoElbow.write(angleElbow);
    servoShoulder.write(angleShoulder);
    servoWrist.write(angleWrist);

    Serial.print("Yaw: ");       Serial.print(valX, 1);
    Serial.print(" Servo1: ");   Serial.print(servoAngleX);
    Serial.print(" | Pitch: ");  Serial.print(valZ, 1);
    Serial.print(" Elbow: ");    Serial.print(angleElbow);
    Serial.print(" Shoulder: "); Serial.print(angleShoulder);
    Serial.print(" Wrist: ");    Serial.println(angleWrist);

  } else if (packetSize > 0) {
    udp.flush();
  }
}