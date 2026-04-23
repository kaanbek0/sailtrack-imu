#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h>

// Pins for Waveshare ESP32 Board
#define EPD_CS    15
#define EPD_DC    27
#define EPD_RST   26
#define EPD_BUSY  25

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

int refreshCounter = 0;   // Keeps track of partial updates
const int MAX_PARTIAL = 60; // Full refresh every 60 seconds (at 1 FPS)

// --- REFRESH FUNCTIONS ---

void drawUI(int data) {
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(50, 50);
  display.print("Heading:");
  display.setCursor(320, 260); // Match position in partial refresh
  display.print(data);
}

void fullRefresh(int data) {
  display.setFullWindow(); // Reset the driver to use the whole screen
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawUI(data); // Draw everything (labels + data)
  } while (display.nextPage());
}

void partialRefresh(int data) {
  // Define only the area where the numbers change
  uint16_t x=300, y=200, w=200, h=100;
  
  display.setPartialWindow(x, y, w, h);
  display.firstPage();
  do {
    display.fillRect(x, y, w, h, GxEPD_WHITE); // Clear the old number
    display.setCursor(x + 20, y + 60);
    display.setFont(&FreeSansBold18pt7b);
    display.print(data);
  } while (display.nextPage());
}


void setup() {
  display.init(115200);
  // Initial clear
  fullRefresh(0); 
}

void loop() {
  int currentData = random(0, 360); // Simulating live heading data

  if (refreshCounter >= MAX_PARTIAL) {
    // Time for the "Clean"
    fullRefresh(currentData);
    refreshCounter = 0; 
  } else {
    // Fast update
    partialRefresh(currentData);
    refreshCounter++;
  }

  delay(1000); // Maintain roughly 1 FPS
}

