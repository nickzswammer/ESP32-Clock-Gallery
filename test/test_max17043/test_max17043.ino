#include "MAX17043.h"
#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection_new_style.h"

void setup()
{
  
  //
  // Initialize the serial interface.
  //
  Serial.begin(115200);
  delay(250);

  display.init(115200);
  display.setRotation(2);
  display.setFullWindow();
  //
  // Wait for serial port to connect.
  //
  while (!Serial) {}
  Serial.println("Serial port initialized.\n");

  //
  // Initialize the fuel gauge.
  //
  if (FuelGauge.begin())
  {
    //
    // Reset the device.
    //
    Serial.println("Resetting device...");
    FuelGauge.reset();
    delay(250);

    //
    // Issue a quickstart command and wait
    // for the device to be ready.
    //
    Serial.println("Initiating quickstart mode...");
    FuelGauge.quickstart();
    delay(125);

    //
    // Display an initial reading.
    //
    Serial.println("Reading device...");
    Serial.println();
    displayReading();
  }
  else
  {
    Serial.println("The MAX17043 device was NOT found.\n");
    while (true);
  }
}

void loop()
{
    displayReading();
    delay(2000);
}

void displayReading()
{
  //
  // Get the voltage, battery percent
  // and other properties.
  //


  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);  // clear the display
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(1);

  display.setCursor(60, 20);  // starting position (x,y)
  display.print("Address:       0x");
  display.setCursor(100, 20);  // starting position (x,y)
  display.print(FuelGauge.address(), HEX);
  display.setCursor(60, 30);  // starting position (x,y)
  display.print("Version:       "); 
  display.setCursor(100, 30);  // starting position (x,y)
  display.print(FuelGauge.version());

  display.setCursor(60, 40);  // starting position (x,y)
  display.print("ADC:           "); 

  display.setCursor(100, 40);  // starting position (x,y)

  display.print(FuelGauge.adc());
  
  display.setCursor(60, 50);  // starting position (x,y)
  display.print("Voltage:       "); 
  
  display.setCursor(100, 50);  // starting position (x,y)  
  display.print(FuelGauge.voltage()); 
  display.setCursor(150, 50);  // starting position (x,y)  

  display.print(" mV");

  display.setCursor(60, 60);  // starting position (x,y)
  display.print("Percent:       "); 
  
  display.setCursor(100, 60);  // starting position (x,y)
  display.print(FuelGauge.percent());
  display.setCursor(150, 60);  // starting position (x,y)
  display.print("%");


  display.setCursor(60, 70);  // starting position (x,y)
  display.print("Is Sleeping:   "); 
  
  display.setCursor(100, 70);  // starting position (x,y)
  display.print(FuelGauge.isSleeping() ? "Yes" : "No");

  display.setCursor(60, 80);  // starting position (x,y)
  display.print("Alert:         "); 
  
  display.setCursor(100, 80);  // starting position (x,y)
  display.print(FuelGauge.alertIsActive() ? "Yes" : "No");

  display.setCursor(60, 90);  // starting position (x,y)
  display.print("Threshold:     "); 

  display.setCursor(100, 90);  // starting position (x,y)
  display.print(FuelGauge.getThreshold()); 
  display.setCursor(150, 90);  // starting position (x,y)

  display.print("%");


    display.setCursor(60, 100);  // starting position (x,y)
  display.print("Compensation:  0x"); 

  display.setCursor(100, 100);  // starting position (x,y)

  display.print(FuelGauge.compensation(), HEX);

  } while (display.nextPage());
}

void displayMenu() {
  String menu_elements[MENU_ITEMS] = { "Sleep (Show Gallery)", "Sleep (Show Clock)", "Hibernation (display one file)" };
  uint8_t curr_y_pos = 100;

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);  // clear the display
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold24pt7b);
    display.setTextSize(1);
    display.setCursor(60, 50);  // starting position (x,y)
    display.print("Menu");

    display.setFont(&FreeSans9pt7b);

    for (int i = 0; i < MENU_ITEMS; i++) {
      if (i == menu_selected_index) {
        display.setCursor(45, curr_y_pos - 2);
        display.print(">");
      }
      display.setCursor(60, curr_y_pos);
      display.print(menu_elements[i].c_str());
      curr_y_pos += 30;
    }

  } while (display.nextPage());
}