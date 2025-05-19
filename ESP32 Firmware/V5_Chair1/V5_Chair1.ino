#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_light.h>

const char* chairName = "Chair 1"; // Change to "Chair 2" on the other chair
uint8_t chair3Mac[] = {0x94, 0xE6, 0x86, 0x48, 0x1D, 0xF8};  // Replace with Chair 3's actual MAC

MPU6050 mpu(Wire);
bool emergencyTriggered = false;
const float EMERGENCY_THRESHOLD = 35.0;
unsigned long timer = 0;

typedef struct {
  char name[10];
  float angleX;
  float angleY;
  float angleZ;
  bool emergency;
} SensorData;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Needed for ESP-NOW to work properly

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, chair3Mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Chair 3 as peer");
  }

  if (mpu.begin() != 0) {
    Serial.println("MPU init failed");
    while (1);
  }
  mpu.calcOffsets();
  mpu.setFilterGyroCoef(0.90);
}

void loop() {
  mpu.update();
  float x = mpu.getAngleX();
  float y = mpu.getAngleY();
  float z = mpu.getAngleZ();

  SensorData data;
  strncpy(data.name, chairName, sizeof(data.name));
  data.angleX = x;
  data.angleY = y;
  data.angleZ = z;
  data.emergency = (abs(x) > EMERGENCY_THRESHOLD || abs(y) > EMERGENCY_THRESHOLD || abs(z) > EMERGENCY_THRESHOLD);

  esp_err_t result = esp_now_send(chair3Mac, (uint8_t*)&data, sizeof(data));
  if (result != ESP_OK) {
    Serial.println("Send failed");
  }

  if ((millis() - timer) > 500) {
    Serial.printf("Sent: %s | X: %.2f Y: %.2f Z: %.2f | Emergency: %d\n", data.name, x, y, z, data.emergency);
    timer = millis();
  }

  delay(10);
}
