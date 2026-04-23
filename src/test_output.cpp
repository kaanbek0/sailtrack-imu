#include <CAN.h>
#include "Protocol.h"


CAN_IMU_Frame imu_X, imu_Y, imu_Z; //v1 = roll or pitch or yaw,  v2 = linear acceleration
CAN_GPS_POS gps_pos;
CAN_GPS_MOTION gps_mot;
CAN_GPS_INFO gps_info;

double lat, lng;


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
        Serial.printf("[IMU] Roll: %.2f | Pitch: %.2f   | Yaw: %.2f\n", imu_X.v1, imu_Y.v1, imu_Z.v1);
        break;
    case ID_GPS_POS:{
        CAN.readBytes((uint8_t *)&gps_pos, sizeof(gps_pos));
        lat = gps_pos.lat/ 1000000.0;
        lng = gps_pos.lng/ 1000000.0;
        break;
    }
    case ID_GPS_MOTION:
        CAN.readBytes((uint8_t *)&gps_mot, sizeof(gps_mot));
        break;
    case ID_GPS_INFO:
        CAN.readBytes((uint8_t *)&gps_info, sizeof(gps_info));
            Serial.printf("[GPS] Sat: %d     |Lat: %.6f | Lng: %.6f | Headding: %.3f | Time: %d\n",gps_info.sats, gps_pos.lat/ 1000000.0, gps_pos.lng/ 1000000.0, gps_mot.headMot, gps_info.time);
            Serial.println("__________________________________________________________________");
        break;
    default:
        Serial.printf("Unknown ID: 0x%03X received\n", id);
        break;
    }
    
  }
}