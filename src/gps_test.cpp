#include <CAN.h>
#include <LoRa.h>
#include <SPI.h>
#include "Protocol.h" //[cite: 1]

// T-Beam LoRa Pin Definitions
#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     23
#define DIO0    26

// Data storage from test_output_5.cpp[cite: 1]
CAN_IMU_Frame imu_X, imu_Y, imu_Z; 
CAN_GPS_POS gps_pos;
CAN_GPS_MOTION gps_mot;
CAN_GPS_INFO gps_info;
double lat, lng;

// This struct packs the data into a single 29-byte message for the radio
struct __attribute__((__packed__)) BoatPacket {
  float roll, pitch, yaw;
  int32_t lat, lng;
  float heading;
  uint8_t sats;
  uint32_t time;
} boatPacket;

void setup() {
  Serial.begin(115200); 
  while (!Serial); //[cite: 1]

  // Initialize CAN with your specific Pins (33 for RX, 25 for TX)
  CAN.setPins(33, 25); 

  if (!CAN.begin(500E3)) { //[cite: 1]
    Serial.println("Starting CAN failed!");
    while (1); //[cite: 1]
  }

  // Initialize T-Beam LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(868E6)) { // Set to your frequency (868MHz)
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("T-Beam Ready: Sending CAN data to Pi via LoRa...");
}

void loop() {
  int packetSize = CAN.parsePacket(); //[cite: 1]

  if (packetSize > 0) {
    long id = CAN.packetId(); //[cite: 1]

    switch (id)
    {
    case ID_IMU_X:
        CAN.readBytes((uint8_t *)&imu_X, sizeof(imu_X)); //[cite: 1]
        boatPacket.roll = imu_X.v1;
        break;
    case ID_IMU_Y:
        CAN.readBytes((uint8_t *)&imu_Y, sizeof(imu_Y)); //[cite: 1]
        boatPacket.pitch = imu_Y.v1;
        break;
    case ID_IMU_Z:
        CAN.readBytes((uint8_t *)&imu_Z, sizeof(imu_Z)); //[cite: 1]
        boatPacket.yaw = imu_Z.v1;
        // Keep original serial output[cite: 1]
        Serial.printf("[IMU] Roll: %.2f | Pitch: %.2f   | Yaw: %.2f\n", imu_X.v1, imu_Y.v1, imu_Z.v1); //[cite: 1]
        break;
    case ID_GPS_POS:{
        CAN.readBytes((uint8_t *)&gps_pos, sizeof(gps_pos)); //[cite: 1]
        boatPacket.lat = gps_pos.lat;
        boatPacket.lng = gps_pos.lng;
        lat = gps_pos.lat/ 1000000.0; //[cite: 1]
        lng = gps_pos.lng/ 1000000.0; //[cite: 1]
        break;
    }
    case ID_GPS_MOTION:
        CAN.readBytes((uint8_t *)&gps_mot, sizeof(gps_mot)); //[cite: 1]
        boatPacket.heading = gps_mot.headMot;
        break;
    case ID_GPS_INFO:
        CAN.readBytes((uint8_t *)&gps_info, sizeof(gps_info)); //[cite: 1]
        boatPacket.sats = gps_info.sats;
        boatPacket.time = gps_info.time;

        // Keep original serial output[cite: 1]
        Serial.printf("[GPS] Sat: %d     |Lat: %.6f | Lng: %.6f | Headding: %.3f | Time: %d\n",gps_info.sats, gps_pos.lat/ 1000000.0, gps_pos.lng/ 1000000.0, gps_mot.headMot, gps_info.time); //[cite: 1]
        Serial.println("__________________________________________________________________"); //[cite: 1]

        // --- BROADCAST TO PI ---
        LoRa.beginPacket();
        LoRa.write((uint8_t *)&boatPacket, sizeof(boatPacket));
        LoRa.endPacket();
        break;
    default:
        Serial.printf("Unknown ID: 0x%03X received\n", id); //[cite: 1]
        break;
    }
  }
}