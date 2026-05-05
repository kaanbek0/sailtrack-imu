#include <Arduino.h>

// E32 Default Pins for ESP32
#define RXD2 16
#define TXD2 17

void setup() {
  // USB Debugging
  Serial.begin(115200);
  
  // LoRa Module (E32 default is 9600 baud)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("📡 Radio Test: Starting Broadcaster...");
}

void loop() {
  // Generate a random "Sensor" value
  int fake_data = random(0, 100);
  
  // Create a simple string message
  String msg = "BOAT_TEST,Value:" + String(fake_data) + "\n";

  // Send to LoRa
  Serial2.print(msg);

  // Debug to USB
  Serial.print("Sending to Shore: ");
  Serial.print(msg);

  delay(2000); // 2 second interval
}