// GxEPD2 ESP32 Audrey Project by Nicholas Zhang
//
// This renders a bitmap stored in images.h
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
/*
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// or select the display constructor line in one of the following files (old style):
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

// images bitmaps file
#include "images.h"

void setup()
{
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  renderBitmap();
  display.hibernate();
}

void renderBitmap()
{
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();

  int y = (300 - 193) / 2;

  do
  {
    display.fillScreen(GxEPD_WHITE);

    
    //display.drawXBitmap(0, y, guitar_dithered_data, 200, 193, GxEPD_BLACK);
    //display.drawXBitmap(200, y, guitar_regular_data, 200, 193, GxEPD_BLACK);
    
    display.drawXBitmap(0, 0, sheldon_bits, 226, 400, GxEPD_BLACK);
    display.drawXBitmap(0, 0, sheldon_dithered_bits, 226, 400, GxEPD_BLACK);
    display.drawXBitmap(0, 0, sheldon_dithered_bleed_bits, 226, 400, GxEPD_BLACK);


  }
  while (display.nextPage());
}

void loop() {};
*/
