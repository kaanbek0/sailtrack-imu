#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_AHRS.h>
#include <Adafruit_Sensor_Calibration.h>
#include <CAN.h>
#include <TinyGPS++.h>
#include <esp_task_wdt.h> 
#include "Protocol.h"

// --- Configuration ---
#define LED_PIN 2 
#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 25
#define GPS_BAUD 9600
#define RECOVERY_INTERVAL 10000 // 10 seconds

// --- Global Objects ---
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
Adafruit_NXPSensorFusion filter;
Adafruit_Sensor_Calibration_EEPROM cal;

// --- State Variables ---
bool imuFound = false;
bool canFound = false;
bool gpsFound = false;
unsigned long lastHeartbeat = 0;
bool ledState = LOW;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 500;
unsigned long lastRecoveryAttempt = 0;

float linearAccelX, linearAccelY, linearAccelZ;

// --- CAN Helper Functions ---
void Send_CAN_IMU(uint32_t ID, float v1, float v2) {
    if (!canFound) return;
    CAN_IMU_Frame msg = {v1, v2};
    CAN.beginPacket(ID);
    CAN.write((uint8_t *)&msg, sizeof(msg));
    CAN.endPacket();
    Serial.printf("Imu data sent with data: %f | %f\n",v1,v2);
}

void Send_CAN_GPS_POS(uint32_t ID, int32_t v1, int32_t v2) {
    if (!canFound) return;
    CAN_GPS_POS msg = {v1, v2};
    CAN.beginPacket(ID);
    CAN.write((uint8_t *)&msg, sizeof(msg));
    CAN.endPacket();
    Serial.printf("gps pos data sent with data: %d | %d\n",v1,v2);
}

void Send_CAN_GPS_MOT(uint32_t ID, float v1, float v2) {
    if (!canFound) return;
    CAN_GPS_MOTION msg = {v1, v2};
    CAN.beginPacket(ID);
    CAN.write((uint8_t *)&msg, sizeof(msg));
    CAN.endPacket();
    Serial.printf("gps mot data sent with data: %f | %f\n",v1,v2);
}

void Send_CAN_GPS_INFO(uint32_t ID, uint32_t v1, uint8_t v2) {
    if (!canFound) return;
    CAN_GPS_INFO msg = {v1, v2};
    CAN.beginPacket(ID);
    CAN.write((uint8_t *)&msg, sizeof(msg));
    CAN.endPacket();
    Serial.printf("gps info data sent with data: %d | %d\n",v1,v2);
}

// --- Recovery Logic ---
void attemptModuleRecovery() {
    if (millis() - lastRecoveryAttempt < RECOVERY_INTERVAL) return;
    lastRecoveryAttempt = millis();

    if (!imuFound) {
        Serial.println("[RECOVERY] Trying IMU...");
        Wire.end(); 
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.setTimeOut(50);
        if (lsm.begin()) {
            lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
            lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
            lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
            imuFound = true;
            Serial.println("[RECOVERY] IMU Success!");
        }
    }

    if (!canFound) {
        Serial.println("[RECOVERY] Trying CAN...");
        CAN.end();
        if (CAN.begin(500E3)) {
            canFound = true;
            Serial.println("[RECOVERY] CAN Success!");
        }
    }

    if (!gpsFound) {
        if (SerialGPS.available() > 0) {
            gpsFound = true;
            Serial.println("[RECOVERY] GPS Success!");
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    // 1. Classic Watchdog Setup (5 seconds)
    esp_task_wdt_init(5, true); 
    esp_task_wdt_add(NULL);

    // 2. I2C Setup
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setTimeOut(50);

    // 3. Sensor Init
    Serial.print("IMU Init... ");
    if (lsm.begin()) {
        lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
        lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
        lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
        imuFound = true;
        Serial.println("OK");
    } else { Serial.println("FAIL"); }

    Serial.print("CAN Init... ");
    if (CAN.begin(500E3)) {
        canFound = true;
        Serial.println("OK");
    } else { Serial.println("FAIL"); }

    cal.begin();
    cal.loadCalibration();
    filter.begin(10); // 10Hz filter

    SerialGPS.begin(GPS_BAUD, SERIAL_8N1, 16, 17);
    
    // Boot Indicator
    for(int i=0; i<3; i++) {
        digitalWrite(LED_PIN, HIGH); delay(100);
        digitalWrite(LED_PIN, LOW);  delay(100);
    }
}

void loop() {
    esp_task_wdt_reset(); // Feed the dog

    // 1. Heartbeat
    if (millis() - lastHeartbeat >= 500) {
        lastHeartbeat = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }

    // 2. Run Recovery Medic
    attemptModuleRecovery();

    // 3. Process IMU
    if (imuFound) {
        sensors_event_t a, m, g, t;
        if (lsm.getEvent(&a, &m, &g, &t)) {
            filter.update(g.gyro.x * SENSORS_RADS_TO_DPS, g.gyro.y * SENSORS_RADS_TO_DPS, g.gyro.z * SENSORS_RADS_TO_DPS,
                          a.acceleration.x, a.acceleration.y, a.acceleration.z,
                          m.magnetic.x, m.magnetic.y, m.magnetic.z);
            filter.getLinearAcceleration(&linearAccelX, &linearAccelY, &linearAccelZ);
        } else {
            imuFound = false; // Mark for recovery
        }
    }

    // 4. Process GPS
    while (SerialGPS.available() > 0) {
        if (gps.encode(SerialGPS.read())) {
            gpsFound = true;
        }
    }

    // 5. Send Data
    if (millis() - lastSendTime >= sendInterval) {
        lastSendTime = millis();

        if (canFound) {
            if (imuFound) {
                Send_CAN_IMU(ID_IMU_X, filter.getRoll(), linearAccelX);
                delayMicroseconds(500);
                Send_CAN_IMU(ID_IMU_Y, filter.getPitch(), linearAccelY);
                delayMicroseconds(500);
                Send_CAN_IMU(ID_IMU_Z, filter.getYaw(), linearAccelZ);
                delayMicroseconds(500);
            }

            if (gpsFound) {
                int32_t latFixed = (int32_t)(gps.location.lat() * 1000000);
                int32_t lngFixed = (int32_t)(gps.location.lng() * 1000000);
                
                Send_CAN_GPS_POS(ID_GPS_POS, latFixed, lngFixed);
                delayMicroseconds(500);
                Send_CAN_GPS_MOT(ID_GPS_MOTION, gps.speed.kmph(), gps.course.deg());
                delayMicroseconds(500);
                Send_CAN_GPS_INFO(ID_GPS_INFO, gps.time.value(), (uint8_t)gps.satellites.value());
            }
        }
    }
}