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
VectorInt16 aa;
VectorFloat gravity;
float ypr[3];

// Tare offsets
float yawOffset = 0, pitchOffset = 0, rollOffset = 0;
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
  Serial.println("MPU6050 connected.");

  Serial.println("Send any character to calibrate and start...");
  while (Serial.available() && Serial.read());
  while (!Serial.available());
  while (Serial.available() && Serial.read());

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
    Serial.println("Ready! Hold hand in neutral position and PINCH to zero.");
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

  float yaw   = ypr[0] * 180.0 / M_PI;
  float pitch = ypr[1] * 180.0 / M_PI;
  float roll  = ypr[2] * 180.0 / M_PI;

  bool isPinching = (digitalRead(BUTTON_PIN) == LOW);

  if (isPinching) {
    if (!wasPinching) {
      // Tare: average 20 samples for a stable zero point
      float sumY = 0, sumP = 0, sumR = 0;
      for (int i = 0; i < 20; i++) {
        while (!mpu.dmpGetCurrentFIFOPacket(FIFOBuffer));
        mpu.dmpGetQuaternion(&q, FIFOBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        sumY += ypr[0] * 180.0 / M_PI;
        sumP += ypr[1] * 180.0 / M_PI;
        sumR += ypr[2] * 180.0 / M_PI;
        delay(10);
      }
      yawOffset   = sumY / 20.0;
      pitchOffset = sumP / 20.0;
      rollOffset  = sumR / 20.0;

      wasPinching = true;
      lastPrintTime = millis();
      Serial.println("--- PINCHED: ANGLES ZEROED ---");
    }

    // Relative angles from tare point
    float relYaw   = yaw   - yawOffset;
    float relPitch = pitch - pitchOffset;
    float relRoll  = roll  - rollOffset;

    // Wrap yaw to -180..180
    if (relYaw >  180) relYaw -= 360;
    if (relYaw < -180) relYaw += 360;

    // Print 10x per second — smooth enough for servo control
    if (millis() - lastPrintTime >= 50) {
      Serial.print("YAW:");   Serial.print(relYaw,   1);
      Serial.print("\tPITCH:"); Serial.print(relPitch, 1);
      Serial.print("\tROLL:");  Serial.print(relRoll,  1);
      Serial.println();
      lastPrintTime = millis();
    }

    float m1 = relYaw * 2;
    float m2 = relRoll;

    Serial.print(m1); Serial.print("this is motor 1 movement");
    Serial.print(m2); Serial.print("this is motor 2 movement");

  } else {
    if (wasPinching) {
      wasPinching = false;
      Serial.println("--- RELEASED ---");
    }
  }
}