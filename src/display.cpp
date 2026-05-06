#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <CAN.h>
#include "Protocol.h"
#include "images.h"

// --- Professional Digital Font ---
// This font is now used for BOTH labels and numbers
#include <DSEG14Classic_Bold20pt7b.h> 
#include <DSEG14Classic_Regular40pt7b.h>

// --- PINS (Preserved Verbatim) ---
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

float current_sog = 0, current_yaw = 0, current_pth = 0, current_rll = 0;
int refresh_count = 0;
bool isAsleep = false;
unsigned long bootTime = 0;

// UI Helper: Draws 7-Segment vertical labels on the far left
void drawVerticalLabel(int x, int y, const char* text) {
    display.setFont(&DSEG14Classic_Bold20pt7b); 
    display.setTextSize(1); // Smaller size for labels, still 7-segmented
    for(int i = 0; i < strlen(text); i++) {
        display.setCursor(x, y + (i * 40)); 
        display.print(text[i]);
    }
}

// UI Helper: Draws the huge 7-Segment numbers
void drawBigValue(int x, int y, float val, int decimals,int size) {
    display.setFont(&DSEG14Classic_Regular40pt7b);
    display.setTextSize(size); 
    display.setCursor(x, y);
    display.print(val, decimals);
}

void syncCAN() {
    while (CAN.parsePacket()) {
        long id = CAN.packetId();
        if (id == ID_IMU_X) {
            CAN_IMU_Frame frame; CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_rll = frame.v1; // RLL[cite: 4]
        } 
        else if (id == ID_IMU_Y) {
            CAN_IMU_Frame frame; CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_pth = frame.v1; // PTH[cite: 4]
        }
        else if (id == ID_IMU_Z) {
            CAN_IMU_Frame frame; CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_yaw = frame.v1; // DRF[cite: 4]
        }
        else if (id == ID_GPS_MOTION) {
            CAN_GPS_MOTION frame; CAN.readBytes((uint8_t *)&frame, sizeof(frame));
            current_sog = frame.kmph; // SOG[cite: 4]
        }
    }
}

void goToSleep() {
    display.setRotation(1); 
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(140, 80, epd_bitmap_metisvela, 200, 200, GxEPD_BLACK); //
        display.drawBitmap(121, 300, epd_bitmap_sailtrack_logo, 238, 175, GxEPD_BLACK); //
        
        // --- BACK TO ORIGINAL FONT FOR SLEEP TEXT ---
        display.setFont(); 
        display.setTextSize(3);
        display.setCursor(120, 550);
        display.print("SHUTTING DOWN");
    } while (display.nextPage());

    delay(3000);
    display.firstPage();
    do { display.fillScreen(GxEPD_WHITE); } while (display.nextPage());
    display.hibernate();
    digitalWrite(EPD_PWR, LOW);
    isAsleep = true;
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

    // Startup Sequence with Logos[cite: 3]
    display.setRotation(1); 
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(140, 80, epd_bitmap_metisvela, 200, 200, GxEPD_BLACK);
        display.drawBitmap(121, 300, epd_bitmap_sailtrack_logo, 238, 175, GxEPD_BLACK);
    } while (display.nextPage());

    CAN.setPins(CAN_RX_PIN, CAN_TX_PIN);
    CAN.begin(500E3);
    delay(2000); 
}

void loop() {
    if (isAsleep) return;
    if (millis() - bootTime > 10000 && digitalRead(SLEEP_BUTTON) == LOW) {
        goToSleep(); return;
    }

    syncCAN();

    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 500) {
        lastRefresh = millis();
        if (refresh_count >= 50) { 
            display.setFullWindow();
            display.firstPage();
            do { display.fillScreen(GxEPD_WHITE); 
                
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);

            // --- SECTION 1: SOG ---
            drawVerticalLabel(10, 60, "SOG"); 
            drawBigValue(80, 160, current_sog, 1, 2);

            // --- SECTION 2: YAW ---
            drawVerticalLabel(10, 260, "YAW");
            drawBigValue(80, 360, current_yaw, 0,2);

            // --- SECTION 3: PTH ---
            drawVerticalLabel(10, 460, "PTH");
            drawBigValue(80, 560, current_pth, 1,2);

            // --- SECTION 4: RLL ---
            drawVerticalLabel(10, 660, "RLL");
            drawBigValue(80, 760, current_rll, 1,2);
            } while (display.nextPage());
            refresh_count = 0;
        }
        else{
        display.setPartialWindow(0, 0, display.width(), display.height());
        display.firstPage();
        do {
            syncCAN();
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);

            // --- SECTION 1: SOG ---
            drawVerticalLabel(10, 60, "SOG"); 
            drawBigValue(80, 160, current_sog, 1, 2);

            // --- SECTION 2: YAW ---
            drawVerticalLabel(10, 260, "YAW");
            drawBigValue(80, 360, current_yaw, 0,2);

            // --- SECTION 3: PTH ---
            drawVerticalLabel(10, 460, "PTH");
            drawBigValue(80, 560, current_pth, 1,2);

            // --- SECTION 4: RLL ---
            drawVerticalLabel(10, 660, "RLL");
            drawBigValue(80, 760, current_rll, 1,2);

        } while (display.nextPage());
        }
        refresh_count++;
    }
}