#include <Arduino.h>
#include <CAN.h>
#include "Protocol.h" //[cite: 5]

// --- E32 Radio Configuration ---
#define RXD2 16
#define TXD2 17
#define RADIO_BAUD 9600

// Data structures from Protocol.h[cite: 5]
CAN_IMU_Frame imu_X, imu_Y, imu_Z; 
CAN_GPS_POS gps_pos;
CAN_GPS_MOTION gps_mot;
CAN_GPS_INFO gps_info;

double lat, lng;

void setup() {
  // 1. Laptop Serial Monitor (Keep original 115200)
  Serial.begin(115200); 
  while (!Serial);

  // 2. LoRa Radio (Serial2)
  Serial2.begin(RADIO_BAUD, SERIAL_8N1, RXD2, TXD2);

  // 3. CAN Bus Setup (Keep original pins 33, 25)
  CAN.setPins(33, 25);
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop() {
  int packetSize = CAN.parsePacket();

  if (packetSize > 0) {
    long id = CAN.packetId();

    switch (id) {
      case ID_IMU_X:
          CAN.readBytes((uint8_t *)&imu_X, sizeof(imu_X));
          // Send raw data to Radio immediately
          Serial2.printf("IMU_X,%.2f,%.2f\n", imu_X.v1, imu_X.v2);
          break;

      case ID_IMU_Y:
          CAN.readBytes((uint8_t *)&imu_Y, sizeof(imu_Y));
          // Send raw data to Radio immediately
          Serial2.printf("IMU_Y,%.2f,%.2f\n", imu_Y.v1, imu_Y.v2);
          break;

      case ID_IMU_Z:
          CAN.readBytes((uint8_t *)&imu_Z, sizeof(imu_Z));
          // Send raw data to Radio
          Serial2.printf("IMU_Z,%.2f,%.2f\n", imu_Z.v1, imu_Z.v2);
          
          // KEEP ORIGINAL SERIAL MONITOR OUTPUT
          Serial.printf("[IMU] Roll: %.2f | Pitch: %.2f   | Yaw: %.2f\n", imu_X.v1, imu_Y.v1, imu_Z.v1);
          break;

      case ID_GPS_POS: {
          CAN.readBytes((uint8_t *)&gps_pos, sizeof(gps_pos));
          lat = gps_pos.lat / 1000000.0;
          lng = gps_pos.lng / 1000000.0;
          // Send raw data to Radio
          Serial2.printf("GPS_POS,%.6f,%.6f\n", lat, lng);
          break;
      }

      case ID_GPS_MOTION:
          CAN.readBytes((uint8_t *)&gps_mot, sizeof(gps_mot));
          // Send raw data to Radio
          Serial2.printf("GPS_MOT,%.2f,%.2f\n", gps_mot.kmph, gps_mot.headMot);
          break;

      case ID_GPS_INFO:
          CAN.readBytes((uint8_t *)&gps_info, sizeof(gps_info));
          // Send raw data to Radio
          Serial2.printf("GPS_INFO,%d,%d\n", gps_info.sats, gps_info.time);
          
          // KEEP ORIGINAL SERIAL MONITOR OUTPUT[cite: 4]
          Serial.printf("[GPS] Sat: %d     |Lat: %.6f | Lng: %.6f | Headding: %.3f | Time: %d\n", 
                        gps_info.sats, gps_pos.lat / 1000000.0, gps_pos.lng / 1000000.0, gps_mot.headMot, gps_info.time);
          Serial.println("__________________________________________________________________");
          break;

      default:
          Serial.printf("Unknown ID: 0x%03X received\n", id);
          break;
    }
  }
}