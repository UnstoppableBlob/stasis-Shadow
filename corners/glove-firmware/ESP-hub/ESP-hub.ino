#include <WiFi.h>
#include <WiFiUdp.h>

// --- Hotspot config ---
const char* AP_SSID     = "ESP32-Hub";
const char* AP_PASSWORD = "password123";
const int   UDP_PORT    = 4210;

WiFiUDP udp;
char buf[256];

void setup() {
  Serial.begin(115200);

  WiFi.softAP(AP_SSID, AP_PASSWORD, 6, 0, 4); // <-- inside setup()

  Serial.print("Hotspot started. WROOM IP: ");
  Serial.println(WiFi.softAPIP());

  udp.begin(UDP_PORT);
  Serial.println("Listening on UDP port 4210...");
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(buf, sizeof(buf) - 1);
    buf[len] = '\0';
    Serial.print("Received: ");
    Serial.println(buf);

    
  }
}

