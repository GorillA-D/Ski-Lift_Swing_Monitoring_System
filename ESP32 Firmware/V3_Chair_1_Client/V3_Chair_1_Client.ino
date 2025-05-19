// For Chair 1 (STA client to Chair3 AP + ESP-NOW + UDP)
#include <WiFi.h>
#include <esp_now.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include "esp_wifi.h"


// Chair 3 AP credentials
const char* ssid = "Chair3_AP";
const char* password = "12345678";
const int ESPNOW_WIFI_CHANNEL = 6;

// UDP
WiFiUDP udp;
IPAddress remoteIP(192, 168, 4, 2);  // Processing PC IP
const int udpPort = 12345;

// Emergency detection
bool emergencyTriggered = false;
unsigned long lastBroadcastTime = 0;
const unsigned long BROADCAST_TIMEOUT_MS = 2000;
const float EMERGENCY_THRESHOLD = 35.0;

// MPU6050
MPU6050 mpu(Wire);
unsigned long timer = 0;

String generateEmergencyMessage(float angleX, float angleY, float angleZ) {
  String id = WiFi.macAddress() + "_" + String(millis() / 1000);
  String msg = "EMERGENCY|ID:" + id + "|SRC:Chair1|CAUSE:AngleExceeded" +
               "|X:" + String(angleX) + "|Y:" + String(angleY) + "|Z:" + String(angleZ);
  return msg;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Wire.begin();

  Serial.println("Connecting to Chair3 AP...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());

  // Lock Wi-Fi to Chair3’s channel
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Init MPU
  if (mpu.begin() != 0) {
    Serial.println("MPU init failed");
    while (1);
  }
  delay(1000);
  mpu.upsideDownMounting = true;
  mpu.calcOffsets();
  mpu.setFilterGyroCoef(0.90);
  mpu.calibrateDriftRateZ(5000);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, 6);  // Broadcast
  peerInfo.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  mpu.update();
  float angleX = mpu.getAngleX();
  float angleY = mpu.getAngleY();
  float angleZ = mpu.getAngleZ();

  // Send angle to PC
  String data = "ANGLE|SRC:Chair1|R:" + String(angleX, 2) +
                "|P:" + String(angleY, 2) +
                "|Y:" + String(angleZ, 2);
  udp.beginPacket(remoteIP, udpPort);
  udp.print(data);
  udp.endPacket();

  // Emergency detection
  if (!emergencyTriggered && (abs(angleX) > EMERGENCY_THRESHOLD ||
                              abs(angleY) > EMERGENCY_THRESHOLD ||
                              abs(angleZ) > EMERGENCY_THRESHOLD)) {
    emergencyTriggered = true;
    String msg = generateEmergencyMessage(angleX, angleY, angleZ);
    Serial.println("EMERGENCY: " + msg);

    esp_now_send(NULL, (uint8_t*)msg.c_str(), msg.length());

    udp.beginPacket(remoteIP, udpPort);
    udp.print(msg);
    udp.endPacket();

    digitalWrite(LED_BUILTIN, HIGH);
    lastBroadcastTime = millis();
  }

  // Reset LED after timeout
  if (emergencyTriggered && millis() - lastBroadcastTime > BROADCAST_TIMEOUT_MS) {
    emergencyTriggered = false;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Emergency ended");
  }

  if ((millis() - timer) > 10) {
    Serial.println(data);
    timer = millis();
  }
}
