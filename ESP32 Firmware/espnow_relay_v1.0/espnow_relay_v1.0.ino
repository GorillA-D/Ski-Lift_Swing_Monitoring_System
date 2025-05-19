#include "ESP32_NOW.h"
#include "WiFi.h"
#include <set>

#define ESPNOW_WIFI_CHANNEL 6
#define EMERGENCY_PIN 0  // Replace with actual pin used

bool emergencyTriggered = false;
unsigned long lastBroadcastTime = 0;
const unsigned long BROADCAST_TIMEOUT_MS = 2000;

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  bool begin() {
    return add();
  }

  bool send_message(const uint8_t *data, size_t len) {
    return send(data, len);
  }
};

ESP_NOW_Broadcast_Peer* broadcaster = nullptr;
std::set<String> seenMessages;

String generateEmergencyMessage() {
  String id = WiFi.macAddress() + "_" + String(millis() / 1000);
  String msg = "EMERGENCY|ID:" + id + "|SRC:" + WiFi.macAddress() + "|CAUSE:Overcurrent";
  seenMessages.insert(id);  // Add to seen immediately
  return msg;
}

String extractMessageID(String msg) {
  int idStart = msg.indexOf("ID:");
  if (idStart == -1) return "";
  int idEnd = msg.indexOf("|", idStart);
  if (idEnd == -1) idEnd = msg.length();
  return msg.substring(idStart + 3, idEnd);
}

void onReceive(const esp_now_recv_info* mac, const unsigned char* data, int len) {
  String msg = String((char*)data).substring(0, len);
  Serial.printf("Received from %02X:%02X:%02X:%02X:%02X:%02X -> %s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], msg.c_str());

  String msgID = extractMessageID(msg);
  if (msgID == "") {
    Serial.println("Invalid message format. Ignored.");
    return;
  }

  if (seenMessages.count(msgID) == 0) {
    seenMessages.insert(msgID);
    Serial.println("New emergency received. Rebroadcasting...");

    if (broadcaster == nullptr) {
      broadcaster = new ESP_NOW_Broadcast_Peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
      broadcaster->begin();
    }

    broadcaster->send_message((uint8_t*)msg.c_str(), msg.length());
  } else {
    Serial.println("Duplicate message. Ignored.");
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(EMERGENCY_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  if (!ESP_NOW.begin()) {
    Serial.println("ESP-NOW initialization failed");
    delay(5000);
    ESP.restart();
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("Setup complete. Listening for emergencies...");
}

void loop() {
  if (digitalRead(EMERGENCY_PIN) == HIGH && !emergencyTriggered) {
    emergencyTriggered = true;

    if (broadcaster == nullptr) {
      broadcaster = new ESP_NOW_Broadcast_Peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
      broadcaster->begin();
    }

    String msg = generateEmergencyMessage();
    Serial.printf("Emergency triggered! Broadcasting: %s\n", msg.c_str());
    broadcaster->send_message((uint8_t*)msg.c_str(), msg.length());

    lastBroadcastTime = millis();
  }

  if (emergencyTriggered && millis() - lastBroadcastTime > BROADCAST_TIMEOUT_MS) {
    Serial.println("Emergency TX finished. Returning to RX-only mode.");
    delete broadcaster;
    broadcaster = nullptr;
    emergencyTriggered = false;
  }

  delay(100);
}