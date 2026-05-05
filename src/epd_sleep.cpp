#include <Arduino.h>

// Standard ESP32 Serial2 pins
#define RXD2 16
#define TXD2 17

void setup() {
  // Laptop Serial Monitor
  Serial.begin(115200);
  
  // LoRa Module (Baud must match the transmitter: 9600)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("📥 Receiver Test Node Active...");
  Serial.println("Listening for the 'Ghost Boat'...");
}

void loop() {
  // If data comes in from the LoRa module...
  if (Serial2.available()) {
    // Read the message
    String message = Serial2.readStringUntil('\n');
    
    // Print it to the laptop
    Serial.print("[RADIO RECEIVED]: ");
    Serial.println(message);
  }
}