#include <Arduino.h>
#include <SPI.h>
#include <SailtrackModule.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_AHRS.h>
#include <Adafruit_Sensor_Calibration.h>

// -------------------------- Configuration -------------------------- //

#define MQTT_PUBLISH_FREQ_HZ		5
#define AHRS_UPDATE_FREQ_HZ			5

#define BATTERY_ADC_PIN 			35
#define BATTERY_ADC_RESOLUTION 		4095
#define BATTERY_ADC_REF_VOLTAGE 	1.1
#define BATTERY_ESP32_REF_VOLTAGE	3.3
#define BATTERY_NUM_READINGS 		32
#define BATTERY_READING_DELAY_MS	20

#define I2C_SDA_PIN 				27
#define I2C_SCL_PIN 				25

#define LOOP_TASK_INTERVAL_MS		5
#define MQTT_TASK_INTERVAL_MS	 	1000 / MQTT_PUBLISH_FREQ_HZ

// ------------------------------------------------------------------- //

SailtrackModule stm;
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
Adafruit_NXPSensorFusion filter;
Adafruit_Sensor_Calibration_EEPROM cal;
float eulerX, eulerY, eulerZ;
float linearAccelX, linearAccelY, linearAccelZ;

class ModuleCallbacks: public SailtrackModuleCallbacks {
	void onStatusPublish(JsonObject status) {
		JsonObject battery = status.createNestedObject("battery");
		float avg = 0;
		for (int i = 0; i < BATTERY_NUM_READINGS; i++) {
			avg += analogRead(BATTERY_ADC_PIN) / BATTERY_NUM_READINGS;
			delay(BATTERY_READING_DELAY_MS);
		}
		battery["voltage"] = 2 * avg / BATTERY_ADC_RESOLUTION * BATTERY_ESP32_REF_VOLTAGE * BATTERY_ADC_REF_VOLTAGE;
	}
};

void mqttTask(void * pvArguments) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	while (true) {
		StaticJsonDocument<STM_JSON_DOCUMENT_MEDIUM_SIZE> doc;

		JsonObject euler = doc.createNestedObject("euler");
		euler["x"] = eulerX;
		euler["y"] = eulerY;
		euler["z"] = eulerZ;

		JsonObject linearAccel = doc.createNestedObject("linearAccel");
		linearAccel["x"] = linearAccelX;
		linearAccel["y"] = linearAccelY;
		linearAccel["z"] = linearAccelZ;

		stm.publish("sensor/imu0", doc.as<JsonObjectConst>());

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(MQTT_TASK_INTERVAL_MS));
	}
}

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
	filter.begin(AHRS_UPDATE_FREQ_HZ);
}

void setup() {
	stm.begin("imu", IPAddress(192, 168, 42, 102), new ModuleCallbacks());
	beginIMU();
	beginAHRS();
	xTaskCreate(mqttTask, "mqttTask", STM_TASK_MEDIUM_STACK_SIZE, NULL, STM_TASK_MEDIUM_PRIORITY, NULL);
}

void loop() {
	TickType_t lastWakeTime = xTaskGetTickCount();
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

	vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(LOOP_TASK_INTERVAL_MS));
}
