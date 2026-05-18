#include <WiFi.h>
#include <WiFiUdp.h>

// --- Must match the WROOM's hotspot ---
const char* AP_SSID     = "ESP32-Hub";
const char* AP_PASSWORD = "password123";

const char* WROOM_IP = "192.168.4.1";
const int   UDP_PORT = 4210;

const int BUTTON_PIN = 21;

WiFiUDP udp;

static int lastState = -1;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(AP_SSID, AP_PASSWORD);
  Serial.print("Connecting to ESP32-Hub");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("ESP3 connected! IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(4212);
  Serial.println("Ready! Waiting for pin changes on GPIO 21 (Binary Mode)...");
}

void loop() {
  // Reconnect if WiFi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.begin(AP_SSID, AP_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Reconnected!");
  }

  bool isPinching = (digitalRead(BUTTON_PIN) == LOW); 
  
  // Create a single byte (unsigned 8-bit integer)
  uint8_t pinState = isPinching ? 1 : 0;

  if (pinState != lastState) {
    lastState = pinState;

    // Send exactly 1 byte over UDP
    udp.beginPacket(WROOM_IP, UDP_PORT);
    udp.write(&pinState, sizeof(pinState));
    udp.endPacket();

    if (isPinching) Serial.println("--- PINCHED (Sent 1) ---");
    else            Serial.println("--- RELEASED (Sent 0) ---");
  }

  delay(50);
}