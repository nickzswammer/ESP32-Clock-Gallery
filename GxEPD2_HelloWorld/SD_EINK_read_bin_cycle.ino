
// GxEPD2 ESP32 Audrey Project by Nicholas Zhang
//
// This renders .bin files stored in a folder in the microSD card. with the first two bits being the height and width
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!

#include <GxEPD2_BW.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// SD Libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

int CS_SD = 2;

void displayImageFromBin(File& file);

void setup() {
  Serial.begin(115200);
  display.init(115200);
  SD.begin(CS_SD); // change this to your SD_CS pin
}

void loop() {
  File root = SD.open("/");

  while (true) {
    File file = root.openNextFile();
    if (!file) {
      root.rewindDirectory(); // start over from beginning
      continue;
    }

    String name = file.name();
    if (!name.endsWith(".bin")) {
      file.close();
      continue;
    }

    Serial.println("Displaying: " + name);
    displayImageFromBin(file);
    file.close();

    delay(7000); // Wait 7 seconds before showing next image
  }
}

void displayImageFromBin(File& file) {
  uint16_t width = file.read() | (file.read() << 8);
  uint16_t height = file.read() | (file.read() << 8);

  int size = (width * height) / 8;
  uint8_t* buffer = (uint8_t*) malloc(size);
  if (!buffer) {
    Serial.println("Memory alloc failed!");
    return;
  }

  file.read(buffer, size);

  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawXBitmap(0, 0, buffer, width, height, GxEPD_BLACK);
  } while (display.nextPage());

  free(buffer);
}
