#include <Arduino.h>
#include <TinyGPS++.h>

// The Neo-M8N default is 9600 baud
#define GPS_BAUD 9600

// Create the TinyGPS++ object
TinyGPSPlus gps;

// Use ESP32 Hardware Serial 2
HardwareSerial SerialGPS(2);

void setup() {
  Serial.begin(115200);
  
  // Initialize GPS Serial: RX=16, TX=17
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, 16, 17);

  Serial.println("Neo-M8N Test: Waiting for satellites...");
  Serial.println("Note: You MUST be near a window or outside!");
}

void loop() {
  // Feed the GPS data into the parser
  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
  }

  // Every 2 seconds, print a status update
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();

    Serial.print("Sats: "); Serial.print(gps.satellites.value());
    
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
}