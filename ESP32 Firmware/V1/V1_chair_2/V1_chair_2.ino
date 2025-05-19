// Code for Chair 2
#include "Wire.h"
#include <MPU6050_light.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <set>

// Constants
#define ESPNOW_WIFI_CHANNEL 6
#define EMERGENCY_THRESHOLD 35.0 // Threshold for pitch, roll, or yaw
#define BROADCAST_TIMEOUT_MS 2000

// MPU6050 Setup
MPU6050 mpu(Wire);
unsigned long timer = 0;

// ESP-NOW Setup
bool emergencyTriggered = false;
unsigned long lastBroadcastTime = 0;

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

// Function to generate an emergency message
String generateEmergencyMessage(float angleX, float angleY, float angleZ) {
  String id = WiFi.macAddress() + "_" + String(millis() / 1000);
  String msg = "EMERGENCY|ID:" + id + "|SRC:" + WiFi.macAddress() +
               "|CAUSE:AngleExceeded|X:" + String(angleX) +
               "|Y:" + String(angleY) + "|Z:" + String(angleZ);
  seenMessages.insert(id); // Add to seen immediately
  return msg;
}

// Function to extract message ID
String extractMessageID(String msg) {
  int idStart = msg.indexOf("ID:");
  if (idStart == -1) return "";
  int idEnd = msg.indexOf("|", idStart);
  if (idEnd == -1) idEnd = msg.length();
  return msg.substring(idStart + 3, idEnd);
}

// Callback for receiving ESP-NOW messages
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
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // Serial and MPU6050 initialization
  Serial.begin(115200);
  Wire.begin();

  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  byte status = mpu.begin();
  if (status != 0) {
    Serial.println("MPU6050 initialization failed!");
    while (1);
  }
  Serial.println("MPU6050 initialized successfully!");

  Serial.println("Calculating offsets, do not move MPU6050...");
  delay(1000);
  mpu.calcOffsets(); // Calibrate gyro and accelerometer
  mpu.setFilterGyroCoef(0.90); // Set complementary filter coefficient
  Serial.println("Offsets calculated!");

  // ESP-NOW initialization
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
  // Update MPU6050 data
  mpu.update();

  // Get angles
  float angleX = mpu.getAngleX();
  float angleY = mpu.getAngleY();
  float angleZ = mpu.getAngleZ();

  // Check for emergency condition
  if (!emergencyTriggered && (abs(angleX) > EMERGENCY_THRESHOLD || abs(angleY) > EMERGENCY_THRESHOLD || abs(angleZ) > EMERGENCY_THRESHOLD)) {
    emergencyTriggered = true;
    
    if (broadcaster == nullptr) {
      broadcaster = new ESP_NOW_Broadcast_Peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
      broadcaster->begin();
    }

    String msg = generateEmergencyMessage(angleX, angleY, angleZ);
    Serial.printf("Emergency triggered! Broadcasting: %s\n", msg.c_str());
    broadcaster->send_message((uint8_t*)msg.c_str(), msg.length());

    lastBroadcastTime = millis();

    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  }

  // Reset emergency state after broadcasting
  if (emergencyTriggered && millis() - lastBroadcastTime > BROADCAST_TIMEOUT_MS) {
    Serial.println("Emergency TX finished. Returning to RX-only mode.");
    delete broadcaster;
    broadcaster = nullptr;
    emergencyTriggered = false;
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off (LOW is the voltage level)
  }

  // Print angles periodically
  if ((millis() - timer) > 10) { // Print data every 10ms
    //Serial.print("X: ");
    Serial.print("\t");
    Serial.print(angleX);
    Serial.print("/");
    Serial.print(angleY);
    Serial.print("/");
    Serial.println(angleZ);
    timer = millis();
  }


}