#include <CAN.h>
#include "Protocol.h"


CAN_IMU_Frame imu_X, imu_Y, imu_Z;


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

    switch (id)
    {
    case ID_IMU_X:
        CAN.readBytes((uint8_t *)&imu_X, sizeof(imu_X));
        break;
    case ID_IMU_Y:
        CAN.readBytes((uint8_t *)&imu_Y, sizeof(imu_Y));
        break;
    case ID_IMU_Z:
        CAN.readBytes((uint8_t *)&imu_Z, sizeof(imu_Z));
        Serial.printf("[IMU] Roll: %.2f | Pitch: %.2f | Yaw: %.2f\n", imu_X.v1, imu_Y.v1, imu_Z.v1);
        break;
    
    default:
        Serial.printf("Unknown ID: 0x%03X received\n", id);
        break;
    }
    
  }
}