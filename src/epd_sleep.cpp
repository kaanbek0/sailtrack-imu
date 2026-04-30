#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>

// 1. PIN DEFINITIONS
#define EPD_CS    2
#define EPD_DC    4
#define EPD_RST   16
#define EPD_BUSY  5
#define EPD_PWR 17

// 2. SPI PINS
#define EPD_SCK   18   
#define EPD_MOSI  15   

// 3. CONSTRUCTOR
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup() {
    Serial.begin(115200);

    // Turn on the board power
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH); 
    delay(100);

    // Start SPI
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    // Initialize display
    display.init(115200);

    // STAGE 1: CLEAR TO WHITE
    // This replicates the epd.Clear() function from your old library
    Serial.println("Clearing screen to white...");
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());

    // STAGE 2: DEEP SLEEP
    Serial.println("Entering Hibernate mode...");
    display.hibernate(); // This sends the software 'Power Off' commands

    // STAGE 3: HARDWARE POWER CUT
    // This replicates the PWR_PIN logic to fully disconnect the HAT
    digitalWrite(EPD_PWR, LOW); 

    Serial.println("System Sleep. Writing is gone, power is off.");
}

void loop() {
    // Everything is asleep.
}