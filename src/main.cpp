#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_AHRS.h>
#include <Adafruit_Sensor_Calibration.h>
#include <CAN.h>
#include "Protocol.h"

//this is a test

//definition for test env
//#define DebugMode

#define I2C_SDA_PIN 				27
#define I2C_SCL_PIN 				25



//IMU definitions
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
Adafruit_NXPSensorFusion filter;
Adafruit_Sensor_Calibration_EEPROM cal;

//global data for output
float eulerX, eulerY, eulerZ;
float linearAccelX, linearAccelY, linearAccelZ;

//message stuctures
void Send_CAN_Frame(uint32_t ID, float v1, float v2)
{
	CAN_IMU_Frame msg = {v1,v2};
	CAN.beginPacket(ID);
	CAN.write((uint8_t *)&msg, sizeof(msg));
	CAN.endPacket();
}

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

	// Sending the messages
	if(millis()-lastSendTime >= sendInterval)
	{
		lastSendTime = millis();

		Send_CAN_Frame(ID_X, filter.getRoll(), linearAccelX);
		delayMicroseconds(500);
		Send_CAN_Frame(ID_Y, filter.getPitch(), linearAccelY);
		delayMicroseconds(500);
		Send_CAN_Frame(ID_Z, filter.getYaw(), linearAccelZ);
	}


	#ifdef DebugMode
	//output for debuging
	Serial.print("Roll: ");
	Serial.println(eulerX);
	Serial.print("Pitch: ");
	Serial.println(eulerY);
	Serial.print("Yaw: ");
	Serial.println(eulerZ);
	Serial.println();
	#endif

}
