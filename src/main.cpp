#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_AHRS.h>
#include <Adafruit_Sensor_Calibration.h>
#include <CAN.h>
#include <TinyGPS++.h>
#include "Protocol.h"



//definition for test env
//#define DebugMode

#define I2C_SDA_PIN 				27
#define I2C_SCL_PIN 				25

// The Neo-M8N default is 9600 baud
#define GPS_BAUD 9600

// Create the TinyGPS++ object
TinyGPSPlus gps;

// Use ESP32 Hardware Serial 2
HardwareSerial SerialGPS(2);


//IMU definitions
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
Adafruit_NXPSensorFusion filter;
Adafruit_Sensor_Calibration_EEPROM cal;

//global data for output
float eulerX, eulerY, eulerZ;
float linearAccelX, linearAccelY, linearAccelZ;

//message sending function
void Send_CAN_IMU(uint32_t ID, float v1, float v2)
{
	CAN_IMU_Frame msg = {v1,v2};
	CAN.beginPacket(ID);
	CAN.write((uint8_t *)&msg, sizeof(msg));
	CAN.endPacket();
}

void Send_CAN_GPS_POS(uint32_t ID, uint32_t v1, uint32_t v2)
{
	CAN_GPS_POS msg = {v1,v2};
	CAN.beginPacket(ID);
	CAN.write((uint8_t *)&msg, sizeof(msg));
	CAN.endPacket();
}

void Send_CAN_GPS_MOT(uint32_t ID, float v1, float v2)
{
	CAN_GPS_MOTION msg = {v1,v2};
	CAN.beginPacket(ID);
	CAN.write((uint8_t *)&msg, sizeof(msg));
	CAN.endPacket();
}

void Send_CAN_GPS_INFO(uint32_t ID, uint32_t v1, uint8_t v2)
{
	CAN_GPS_INFO msg = {v1,v2};
	CAN.beginPacket(ID);
	CAN.write((uint8_t *)&msg, sizeof(msg));
	CAN.endPacket();
}


//message sending timer
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100;


void beginIMU() {
	Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
	lsm.begin();
	lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  	lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  	lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
}

void beginAHRS() {
	cal.begin();
	cal.loadCalibration();
	filter.begin(5);
}

void setup() {
	Serial.begin(115200);
	beginIMU();
	beginAHRS();
	CAN.begin(500E3); 

	// Initialize GPS Serial
	SerialGPS.begin(GPS_BAUD, SERIAL_8N1, 16, 17);
}

void loop() {

	//IMU data
	sensors_event_t accelEvent, gyroEvent, magEvent, tempEvent;

	lsm.getEvent(&accelEvent, &magEvent, &gyroEvent, &tempEvent); 

	cal.calibrate(accelEvent);
	cal.calibrate(gyroEvent);
	cal.calibrate(magEvent);

	float gx, gy, gz;
	gx = gyroEvent.gyro.x * SENSORS_RADS_TO_DPS;
	gy = gyroEvent.gyro.y * SENSORS_RADS_TO_DPS;
	gz = gyroEvent.gyro.z * SENSORS_RADS_TO_DPS;

	float ax, ay, az;
	ax = accelEvent.acceleration.x / SENSORS_GRAVITY_STANDARD;
	ay = accelEvent.acceleration.y / SENSORS_GRAVITY_STANDARD;
	az = accelEvent.acceleration.z / SENSORS_GRAVITY_STANDARD;

	filter.update(gx, gy, gz, ax, ay, az, magEvent.magnetic.x, magEvent.magnetic.y, magEvent.magnetic.z);

	eulerX = filter.getRoll();
	eulerY = filter.getPitch();
	eulerZ = filter.getYaw();

	filter.getLinearAcceleration(&linearAccelX, &linearAccelY, &linearAccelZ); 

	//reading data from the gps
	while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
	}

	// Sending the messages
	#ifndef DebugMode
	if(millis()-lastSendTime >= sendInterval)
	{
		lastSendTime = millis();

		Send_CAN_IMU(ID_IMU_X, filter.getRoll(), linearAccelX);
		delayMicroseconds(500);
		Send_CAN_IMU(ID_IMU_Y, filter.getPitch(), linearAccelY);
		delayMicroseconds(500);
		Send_CAN_IMU(ID_IMU_Z, filter.getYaw(), linearAccelZ);
		delayMicroseconds(500);
		Send_CAN_GPS_POS(ID_GPS_POS, gps.location.rawLat().billionths, gps.location.rawLng().billionths);
		delayMicroseconds(500);
		Send_CAN_GPS_MOT(ID_GPS_MOTION, gps.speed.kmph(), gps.course.deg());
		delayMicroseconds(500);
		Send_CAN_GPS_INFO(ID_GPS_INFO, gps.time.value(), gps.satellites.value());
	}
	#endif


	#ifdef DebugMode
	static unsigned long lastPrint = 0;
  	if (millis() - lastPrint > 2000) {
    lastPrint = millis();
	//output for debuging
	Serial.printf("[IMU] Roll: %.2f | Pitch: %.2f | Yaw: %.2f\n", filter.getRoll(), filter.getPitch(), filter.getYaw());
	Serial.print("[GPS] Sats: "); Serial.print(gps.satellites.value());
    
    if (gps.location.isValid()) {
      Serial.print(" | LAT: "); Serial.print(gps.location.lat(), 6);
      Serial.print(" | LON: "); Serial.print(gps.location.lng(), 6);
      Serial.print(" | Speed (km/h): "); Serial.print(gps.speed.kmph());
      Serial.print(" | Time: "); Serial.print(gps.time.value()); // Similar to epoch
    } else {
      Serial.print(" | Waiting for FIX...");
    }
    
    Serial.println();

    // If you haven't received ANY data after 5 seconds, check wiring
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println("WARNING: No data from GPS. Check TX/RX wiring!");
    }
	}
	#endif

}
