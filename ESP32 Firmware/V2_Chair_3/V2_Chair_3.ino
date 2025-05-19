// For Chair 3 
// Added UDT Communication to allow wirless data streaming between Processing 4 and ESP32
#include <WiFi.h>
#include "esp_wifi.h"
#include <WiFiUdp.h>
#include <esp_now.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <set>

// Wi-Fi credentials
const char* ssid = "Enze";
const char* password = "12345678";
const int ESPNOW_WIFI_CHANNEL = 6; // Must match router


// UDP Setup
WiFiUDP udp;
IPAddress remoteIP(172, 20, 10, 12); // IP of your Processing PC

const int udpPort = 12345;

// Emergency setup
bool emergencyTriggered = false;
unsigned long lastBroadcastTime = 0;
const unsigned long BROADCAST_TIMEOUT_MS = 2000;
const float EMERGENCY_THRESHOLD = 35.0;
std::set<String> seenMessages;

// MPU setup
MPU6050 mpu(Wire);
unsigned long timer = 0;

void onReceive(const esp_now_recv_info* mac, const unsigned char* data, int len) {
  String msg = String((char*)data).substring(0, len);
  Serial.printf("Received from %02X:%02X:%02X:%02X:%02X:%02X -> %s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], msg.c_str());

  int idStart = msg.indexOf("ID:");
  if (idStart == -1) return;
  int idEnd = msg.indexOf("|", idStart);
  if (idEnd == -1) idEnd = msg.length();
  String msgID = msg.substring(idStart + 3, idEnd);

  if (seenMessages.count(msgID) == 0) {
    seenMessages.insert(msgID);
    Serial.println("New emergency received. Rebroadcasting...");
    esp_now_send(NULL, (uint8_t*)msg.c_str(), msg.length());
  } else {
    Serial.println("Duplicate message. Ignored.");
  }
}

String generateEmergencyMessage(float angleX, float angleY, float angleZ) {
  String id = WiFi.macAddress() + "_" + String(millis() / 1000);
  String msg = "EMERGENCY|ID:" + id + "|SRC:" + WiFi.macAddress() +
               "|CAUSE:AngleExceeded|X:" + String(angleX) +
               "|Y:" + String(angleY) + "|Z:" + String(angleZ);
  seenMessages.insert(id);
  return msg;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Wire.begin();


  // MPU6050
  Serial.println("Initializing MPU6050...");
  if (mpu.begin() != 0) {
    Serial.println("MPU init failed");
    while (1);
  }
  delay(1000);
  mpu.upsideDownMounting = true;
  mpu.calcOffsets();
  mpu.setFilterGyroCoef(0.90);
  mpu.calibrateDriftRateZ(5000);
  Serial.println("MPU ready");

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // Ensure correct channel for ESP-NOW
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW failed");
    ESP.restart();
  }
  esp_now_register_recv_cb(onReceive);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress(), 6);
  peerInfo.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  mpu.update();
  float angleX = mpu.getAngleX();
  float angleY = mpu.getAngleY();
  float angleZ = mpu.getAngleZ();

  // Send real-time data over UDP
  String data = "ANGLE|R:" + String(angleX, 2) + 
                "|P:" + String(angleY, 2) + 
                "|Y:" + String(angleZ, 2);
  udp.beginPacket(remoteIP, udpPort);
  udp.print(data);
  udp.endPacket();

  // Check for emergency
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

const uint8_t* broadcastAddress() {
  static uint8_t bcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return bcast;
}
