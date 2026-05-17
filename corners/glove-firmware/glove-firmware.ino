#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

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

void setup() {
  Wire.begin(8, 9);
  Wire.setClock(400000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial);

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
  } else {
    Serial.print("DMP failed, code: ");
    Serial.println(devStatus);
  }
}

void loop() {
  if (!DMPReady) return;
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

    if (millis() - lastPrintTime >= 10) {
      float relYaw  = yaw  - yawOffset;
      float relRoll = roll - rollOffset;

      if (relYaw >  180) relYaw -= 360;
      if (relYaw < -180) relYaw += 360;

      Serial.print(relYaw, 1);
      Serial.print("\t");
      Serial.println(relRoll, 1);

      lastPrintTime = millis();
    }

  } else {
    wasPinching = false;
  }
}