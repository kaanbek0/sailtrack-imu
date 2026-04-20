#include <CAN.h>
#include "Protocol.h"


CAN_IMU_Frame imu_X, imu_Y, imu_Z;
bool newX = false, newY = false, newZ = false;

void setup() {
  Serial.begin(115200); 
  while (!Serial);

  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop() {
  int packetSize = CAN.parsePacket();

  if (packetSize > 0) {
    long id = CAN.packetId();
    uint8_t *ptr = nullptr;

    // 1. Just store the data, DON'T print here.
    if (id == ID_X) { ptr = (uint8_t *)&imu_X; newX = true; }
    else if (id == ID_Y) { ptr = (uint8_t *)&imu_Y; newY = true; }
    else if (id == ID_Z) { ptr = (uint8_t *)&imu_Z; newZ = true; }

    if (ptr != nullptr) {
      for (int i = 0; i < packetSize; i++) {
        if (CAN.available()) ptr[i] = CAN.read();
      }
    }

    // 2. Only print once we have a complete set (all 3 IDs)
    if (newX && newY && newZ) {
      Serial.println("--- Full IMU Update ---");
      Serial.printf("X: %.2f | %.2f\n", imu_X.v1, imu_X.v2);
      Serial.printf("Y: %.2f | %.2f\n", imu_Y.v1, imu_Y.v2);
      Serial.printf("Z: %.2f | %.2f\n", imu_Z.v1, imu_Z.v2);
      
      // Reset flags for the next set
      newX = newY = newZ = false;
    }
  }
}