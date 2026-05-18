#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif
#include <WiFi.h>
#include <WiFiUdp.h>

// --- WiFi / UDP config ---
const char* AP_SSID     = "ESP32-Hub";
const char* AP_PASSWORD = "password123";
const char* WROOM_IP    = "192.168.4.1";
const int   UDP_PORT    = 4210;

WiFiUDP udp;

// --- MPU ---
MPU6050 mpu;

const int BUTTON_PIN = 21;

bool DMPReady = false;
uint8_t devStatus;
uint16_t packetSize;
uint8_t FIFOBuffer[64];

Quaternion q;
VectorFloat gravity;
float ypr[3];

float yawOffset = 0, rollOffset = 0;
bool wasPinching = false;
unsigned long lastPrintTime = 0;

// --- ADDED: The High-Speed Binary Data Structure ---
struct SensorData {
  float yaw;
  float roll;
};

void setup() {
  Wire.begin(8, 9);
  Wire.setClock(400000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial);

  // --- WiFi ---
  WiFi.begin(AP_SSID, AP_PASSWORD);
  Serial.print("Connecting to ESP32-Hub");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  udp.begin(4212);

  // --- MPU ---
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (true);
  }


  devStatus = mpu.dmpInitialize();
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);

  if (devStatus == 0) {
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.setDMPEnabled(true);
    DMPReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
    Serial.println("MPU ready.");
  } else {
    Serial.print("DMP failed, code: ");
    Serial.println(devStatus);
  } 
}

void loop() {
  if (!DMPReady) return;

  // --- WiFi watchdog ---
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(AP_SSID, AP_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
  }

  if (!mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) return;

  mpu.dmpGetQuaternion(&q, FIFOBuffer);
  mpu.dmpGetGravity(&gravity, &q);
  mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

  float yaw  = ypr[0] * 180.0 / M_PI;
  float roll = ypr[2] * 180.0 / M_PI;

  bool isPinching = (digitalRead(BUTTON_PIN) == LOW);

  if (isPinching) {
    if (!wasPinching) {
      // Tare on first pinch frame
      float sumY = 0, sumR = 0;
      for (int i = 0; i < 20; i++) {
        while (!mpu.dmpGetCurrentFIFOPacket(FIFOBuffer));
        mpu.dmpGetQuaternion(&q, FIFOBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        sumY += ypr[0] * 180.0 / M_PI;
        sumR += ypr[2] * 180.0 / M_PI;
        delay(10);
      }
      yawOffset  = sumY / 20.0;
      rollOffset = sumR / 20.0;

      wasPinching = true;
      lastPrintTime = millis();
    }

    if (millis() - lastPrintTime >= 50) {
      float relYaw  = yaw  - yawOffset;
      float relRoll = roll - rollOffset;

      if (relYaw >  180) relYaw -= 360;
      if (relYaw < -180) relYaw += 360;

      relYaw = -relYaw;


      // 1. Create the struct and load it with your data
      SensorData myData;
      myData.yaw = relYaw;
      myData.roll = relRoll;

      // 2. Send the raw binary packet (No strings attached!)
      udp.beginPacket(WROOM_IP, UDP_PORT);
      udp.write((uint8_t*)&myData, sizeof(myData));
      udp.endPacket();

  Serial.print("Sent X: ");
  Serial.print(myData.yaw);
  Serial.print("\tZ: ");
  Serial.println(myData.roll); // mirror to serial for debugging

      lastPrintTime = millis();
    }

  } else {
    wasPinching = false;
  }
}