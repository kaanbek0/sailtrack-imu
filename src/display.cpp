#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <CAN.h>
#include "Protocol.h"
#include "images.h"

// --- PINS (Preserved) ---
#define EPD_CS    2
#define EPD_DC    4
#define EPD_RST   16
#define EPD_BUSY  5
#define EPD_PWR   17
#define EPD_SCK   18   
#define EPD_MOSI  15 
#define CAN_RX_PIN 22
#define CAN_TX_PIN 23
#define SLEEP_BUTTON 27 

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

double current_lat = 0, current_lng = 0;
float current_roll = 0;
int refresh_count = 0;
bool isAsleep = false;
unsigned long bootTime = 0;

void syncCAN() {
    while (CAN.parsePacket()) {
        long id = CAN.packetId();
        if (id == ID_IMU_X) {
            CAN_IMU_Frame frame;
            CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_roll = frame.v1;
        } 
        else if (id == ID_GPS_POS) {
            CAN_GPS_POS frame;
            CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_lat = frame.lat / 1000000.0;
            current_lng = frame.lng / 1000000.0;
        }
    }
}

// --- UPDATED: THE GOODBYE SEQUENCE ---
void goToSleep() {
    Serial.println("Starting Goodbye Sequence...");

    // 1. SHOW LOGO (The Goodbye Screen)
    display.setRotation(1); 
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        // Centered logo (X:140, Y:300)
        display.drawBitmap(140, 300, epd_bitmap_metisvela, 200, 200, GxEPD_BLACK);
        
        display.setTextSize(3);
        display.setCursor(120, 550);
        display.print("SHUTTING DOWN");
    } while (display.nextPage());

    // 2. PAUSE (Wait 3 seconds so you can see the boat)
    delay(3000);

    // 3. THE FINAL CLEANSE (Wash to White to prevent ghosting)
    Serial.println("Performing final white wash...");
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());

    // 4. HARDWARE POWER DOWN
    display.hibernate();
    digitalWrite(EPD_PWR, LOW);
    isAsleep = true;
    Serial.println("SYSTEM HIBERNATING. Safe to unplug.");
}

void setup() {
    Serial.begin(115200);
    bootTime = millis();
    
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH); 
    pinMode(SLEEP_BUTTON, INPUT_PULLUP); 
    
    delay(100);
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
    display.init(115200);

    // Startup Logo
    display.setRotation(1); 
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(140, 300, epd_bitmap_metisvela, 200, 200, GxEPD_BLACK);
    } while (display.nextPage());

    CAN.setPins(CAN_RX_PIN, CAN_TX_PIN);
    CAN.begin(500E3);
    delay(2000); 
}

void loop() {
    if (isAsleep) return;

    // Safety: Ignore button for first 10s to avoid loop if shorted
    if (millis() - bootTime > 10000) { 
        if (digitalRead(SLEEP_BUTTON) == LOW) {
            goToSleep();
            return;
        }
    }

    syncCAN();

    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 500) {
        lastRefresh = millis();

        if (refresh_count >= 20) {
            display.setFullWindow();
            display.firstPage();
            do { display.fillScreen(GxEPD_WHITE); } while (display.nextPage());
            refresh_count = 0;
        }

        syncCAN();
        double dLat = current_lat;
        double dLng = current_lng;
        float dRoll = current_roll;

        display.setRotation(1); 
        display.setPartialWindow(0, 0, display.width(), display.height());

        display.firstPage();
        do {
            syncCAN();
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);

            // SECTION 1: LATITUDE
            display.setTextSize(3);
            display.setCursor(20, 30); display.print("LATITUDE"); 
            display.setTextSize(6);
            display.setCursor(20, 110); display.print(dLat, 6);

            display.drawFastHLine(0, 210, 480, GxEPD_BLACK);

            // SECTION 2: LONGITUDE
            display.setTextSize(3);
            display.setCursor(20, 250); display.print("LONGITUDE");
            display.setTextSize(6);
            display.setCursor(20, 330); display.print(dLng, 6);

            display.drawFastHLine(0, 430, 480, GxEPD_BLACK);

            // SECTION 3: ROLL ANGLE
            display.setTextSize(3);
            display.setCursor(20, 470); display.print("ROLL ANGLE");
            display.setTextSize(9); 
            display.setCursor(60, 630); display.print(dRoll, 1);
            display.setTextSize(4);
            display.setCursor(280, 710); display.print("deg");

        } while (display.nextPage());

        refresh_count++;
    }
}