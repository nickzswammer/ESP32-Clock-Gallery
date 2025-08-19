/*
  Project Title: Audrey Clock Gallery
  File Name:     full_disp_AND_upload.ino
  Author:        Nicholas Zhang
  Date:          May 16, 2025

  Description:
  4 Pushbuttons and gives user interface to select files to display and clock, as well as enabling SD Wifi Uploading
*/

/* ========================== LIBRARIES & FILES ========================== */

#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection_new_style.h"
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <MAX17043.h>

// Wifi Libraries
#include <WiFi.h>            //Built-in
#include <ESP32WebServer.h>  //https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include "esp_wifi.h"

// SD Libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// RTC Library Include
#include "RTClib.h"

// Long Button Press
#include "OneButton.h"

// Local Headers
#include "WebHelpers.h"  //Includes headers of the web and style file
#include "AppState.h"
#include "secrets.h"

// pin definitions
#define CS_SD 2
#define BUTTON_1 26
#define BUTTON_2 25
#define BUTTON_3 33
#define BUTTON_4 32
#define ALRT 26

#define LONG_PRESS_TIME 1500       // milliseconds
#define uS_TO_S_FACTOR 1000000ULL  // 1e06

#define MENU_ITEMS 3

#define DIRECTORY "/bin_images"

AppState app;

// IMPORTANT: SYSTEM STATES
enum { CLOCK_MODE,
       DISPLAY_MODE,
       MENU_MODE,
       SLEEP_MODE } systemState;

/* ========================== VARIABLES ========================== */

ESP32WebServer server(80);
RTC_DS3231 rtc;

String formattedTimeArray[5] = { "Tuesday, May 29, 2025", "Tuesday", "11", "59", "AM" };

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char months[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// for long button presses (B1 B2 B4)
unsigned long button1PressTime = 0;
bool button1Held = false;

unsigned long button2PressTime = 0;
bool button2Held = false;

unsigned long button4PressTime = 0;
bool button4Held = false;

String lastDisplayedTime = "";

// clockvars for update clock stuff
String currentTime;
DateTime now;
uint8_t batteryPercentage;

// timeout vars
const unsigned long SLEEP_TIMEOUT = 2 * 60 * 1000;  // 2 minutes
unsigned long lastActivityTime = 0;

// current menu item
uint8_t menu_selected_index = 0;

// default gallery change interval
uint64_t uploadedGalleryInterval = 10ULL * uS_TO_S_FACTOR;  // default is 10 seconds until user sets (IN MICROSECONDS)
                                                            // ULL prevents overflow so it is an unsigned long long

/* ========================== SETUP ========================== */

void setup() {
  Serial.begin(115200);
  delay(250);

  while (!Serial) {}
  Serial.println("Serial port initialized.\n");

  display.init(115200);
  display.setRotation(2);
  display.setFullWindow();

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  pinMode(ALRT, INPUT_PULLUP);

  lastActivityTime = millis();  // Initialize activity timer

  /* ===================== WIFI BLOCK ===================== */
  restartWiFiAndServer();

  /* ===================== INIT SD CARD ===================== */
  Serial.print(F("Initializing SD card... "));

  //see if the card is present and can be initialised.
  if (!SD.begin(CS_SD)) {
    Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
    app.sdPresent = false;
  } else {
    Serial.println(F("\nCard initialised. File access enabled!"));
    app.sdPresent = true;
  }

  // starts off directory inside /bin_images
  app.root = SD.open(DIRECTORY);
  if (!app.root || !app.root.isDirectory()) {
    Serial.println("ERROR: Failed to open directory");
    return;
  }

  app.currentFile = app.root.openNextFile();
  app.currentFileName = getCurrentFileName();
  app.fileNames[0][0] = getCurrentFileName();

  list_dir(SD, DIRECTORY);

  /* ===================== INIT RTC ===================== */
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  /* ===================== INIT FUEL GUAGE ===================== */
  // Initialize the fuel gauge.
  //
  if (FuelGauge.begin()) {
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
    Serial.printf("Battery Percentage: %d\n", FuelGauge.percent());

    FuelGauge.setThreshold(10);  // set low battery threshold to 10%

  } else {
    Serial.println("The MAX17043 device was NOT found.\n");
  }
}

void loop() {
  // button states
  app.currButton1 = digitalRead(BUTTON_1);
  app.currButton2 = digitalRead(BUTTON_2);
  app.currButton3 = digitalRead(BUTTON_3);
  app.currButton4 = digitalRead(BUTTON_4);

  // updates formatted time array with current RTC readings, also updates battery percentage
  updateClock();

  checkLowBattery(); // checks if batteyr low and displays splash screen

  // Auto-sleep after timeout
  if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
    Serial.println("Inactivity timeout - entering clock sleep mode");
    enterClockSleep();
  }

  // update clock once minute changes in RTC
  if (currentTime != lastDisplayedTime && systemState == CLOCK_MODE) {
    lastDisplayedTime = currentTime;  // update the stored time

    displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);
  }

  //Serial.printf("\nCurrent Formatted Time: %s\n", formatTime(now).c_str());
  //Serial.printf("\nCurrent Time: %s:%s %s\n", formattedTimeArray[2], formattedTimeArray[3], formattedTimeArray[4]);

  //Listen for client connections
  server.handleClient();

  // set file name
  app.currentFileName = getCurrentFileName();

  // display
  // display curr file or toggle menu (TOGGLED ON LONG PRESS)

  // detects button press
  if (!app.currButton1 && app.prevButton1) {
    lastActivityTime = millis();  // Reset timer

    button1PressTime = millis();
    button1Held = false;
  }

  if (!app.currButton1 && (millis() - button1PressTime > LONG_PRESS_TIME)) {
    lastActivityTime = millis();  // Reset timer

    if (!button1Held) {
      button1Held = true;
      Serial.println("\nLong Press For Button 1 Detected: Toggle Menu");

      // toggle systemstate between menu and clock
      systemState = systemState == MENU_MODE ? CLOCK_MODE : MENU_MODE;

      Serial.print("Menu is now ");
      Serial.println(systemState == MENU_MODE ? "Showing" : "Not Showing");

      // if system state is toggled to menu mode, display menu, else show the clock
      systemState == MENU_MODE ? displayMenu() : displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);

      // signify clock mode to serial
      Serial.println(systemState == CLOCK_MODE ? "Device is now in Clock Mode" : "");
    }
  }

  // regular short press behavior
  if (app.wasPressed(app.currButton1, app.prevButton1)) {
    lastActivityTime = millis();  // Reset timer

    if (!button1Held && systemState == DISPLAY_MODE)
      handle_button_1();
  }

  // detects button press
  if (!app.currButton2 && app.prevButton2) {
    lastActivityTime = millis();  // Reset timer

    button2PressTime = millis();
    button2Held = false;
  }

  if (!app.currButton2 && (millis() - button2PressTime > LONG_PRESS_TIME)) {
    lastActivityTime = millis();  // Reset timer

    if (!button2Held) {
      button2Held = true;
      Serial.println("\nLong Press For Button 2 Detected: Resetting Device");

      ESP.restart();
    }
  }

  // select prev file
  if (app.wasPressed(app.currButton2, app.prevButton2 && systemState == DISPLAY_MODE)) {
    lastActivityTime = millis();  // Reset timer

    Serial.println("");

    if (app.fileCount == 0) {
      Serial.println("No files found");
      return;
    }

    if (app.fileMode) {
      app.currentFileIndex--;

      if (app.currentFileIndex < 0)
        app.currentFileIndex = app.fileCount - 1;

      Serial.printf("Selected File Index: [%d]: ", app.currentFileIndex);
    } else {
      // PAGE SELECT MODE: move to previous page
      uint8_t currentPage = getCurrentPage();
      currentPage = (currentPage - 1 + app.totalPages) % app.totalPages;

      app.currentFileIndex = currentPage * MAX_FILES_PER_PAGE;

      Serial.printf("Selected Page Number: #%d\n", currentPage + 1);
    }
    Serial.println(String(app.currentFileIndex + 1) + " " + getCurrentFileName());
    display_files();
  }

  // if menu open, select prev thing
  else if (app.wasPressed(app.currButton2, app.prevButton2 && systemState == MENU_MODE)) {
    lastActivityTime = millis();  // Reset timer

    menu_selected_index = (menu_selected_index - 1 + MENU_ITEMS) % MENU_ITEMS;
    displayMenu();
  }

  // select next file
  if (app.wasPressed(app.currButton3, app.prevButton3) && systemState == DISPLAY_MODE) {
    lastActivityTime = millis();  // Reset timer

    Serial.println("");

    if (app.fileCount == 0) {
      Serial.println("No files found");
      return;
    }

    // FILE SELECT MODE: move onto next image to display
    if (app.fileMode) {
      app.currentFileIndex++;

      if (app.currentFileIndex > app.fileCount - 1)
        app.currentFileIndex = 0;

      Serial.printf("Selected File Number: #[%d]: ", app.currentFileIndex);
    } else {
      // PAGE SELECT MODE: move to next page
      uint8_t currentPage = getCurrentPage();
      currentPage = (currentPage + 1 + app.totalPages) % app.totalPages;
      app.currentFileIndex = currentPage * MAX_FILES_PER_PAGE;

      Serial.printf("Selected Page Number: #%d\n", currentPage + 1);
    }
    Serial.println(String(app.currentFileIndex + 1) + " " + getCurrentFileName());

    display_files();
  }

  // if menu open, select prev thing
  else if (app.wasPressed(app.currButton3, app.prevButton3) && systemState == MENU_MODE) {
    lastActivityTime = millis();  // Reset timer

    menu_selected_index = (menu_selected_index + 1) % MENU_ITEMS;
    displayMenu();
  }

  // show files OR go to clock mode (TOGGLED ON LONG PRESS)

  // detects button press
  if (!app.currButton4 && app.prevButton4) {
    lastActivityTime = millis();  // Reset timer

    button4PressTime = millis();
    button4Held = false;
  }

  if (!app.currButton4 && (millis() - button4PressTime > LONG_PRESS_TIME)) 
  {
    lastActivityTime = millis();  // Reset timer

    if (!button4Held && systemState != MENU_MODE) {
      button4Held = true;
      Serial.println("\nLong Press for Button 4 Detected: Change Modes");

      // toggle systemstate between display and clock
      systemState = systemState == DISPLAY_MODE ? CLOCK_MODE : DISPLAY_MODE;

      Serial.print("Device is now in ");
      Serial.println(systemState == DISPLAY_MODE ? "Display Mode" : "Clock Mode");

      // if system state is toggled to display mode, display all the files, else show the clock
      systemState == DISPLAY_MODE ? display_files() : displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);
    }
  }

  if (app.wasPressed(app.currButton4, app.prevButton4)) 
  {
    lastActivityTime = millis();  // Reset timer

    if (!button4Held && systemState == DISPLAY_MODE) {
      app.fileMode = !app.fileMode;
      if (app.fileMode) {
        Serial.println("Changed to File Select Mode");
      } else {
        Serial.println("Changed to Page Select Mode");
      }
      display_files();
    }
    // if in menu, handle sleep
    // menu_selected_index: (0/1 for Sleep Modes, 2 for hibernation (ONLY DISPLAY ONE IMAGE))
    else if (!button4Held && systemState == MENU_MODE) {
      Serial.printf("Button 4 In Menu Press Detected for: %d\n", menu_selected_index);
      // sleep but show gallery and rotate every uploadedGalleryInterval
      if (menu_selected_index == 0) {
        Serial.println("In Sleep Mode, displaying gallery");
        while (true) {
          constexpr gpio_num_t WAKEUP_PIN = GPIO_NUM_26;        // button 1
          gpio_wakeup_enable(WAKEUP_PIN, GPIO_INTR_LOW_LEVEL);  // Trigger wake-up on high level

          esp_sleep_enable_gpio_wakeup();
          esp_sleep_enable_timer_wakeup(uploadedGalleryInterval);

          if (FuelGauge.percent() < 10)
          {
            break;
          }

          // Stop server and Wi-Fi
          Serial.println("Turning off WiFi...");
          stopWifi();
          Serial.println("WiFi Disabled.");

          Serial.printf("Selected File Number: #[%d]: ", app.currentFileIndex);
          Serial.printf("Printing Current File: %s [%d]\n", getCurrentFileName().c_str(), app.currentFileIndex);
          String current_file_location = String(DIRECTORY) + "/" + getCurrentFileName();
          Serial.printf("At location: %s\n", current_file_location.c_str());
          app.currentFile = SD.open(current_file_location, FILE_READ);
          displayImageFromBin(app.currentFile);

          // select next file
          app.currentFileIndex++;

          // wrap around
          if (app.currentFileIndex > app.fileCount - 1)
            app.currentFileIndex = 0;

          // sleep
          Serial.println("Now Sleeping.");
          delay(500);
          esp_light_sleep_start();

          // After waking
          Serial.println("Woke up!");
          esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

          if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
            lastActivityTime = millis();  // Reset timer
            Serial.println("Button Wake Detected — exiting sleep loop");
            systemState = CLOCK_MODE;

            displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);

            // restart wifi
            Serial.println("Restarting Wifi...");
            restartWiFiAndServer();
            Serial.println("Restarting Wifi completed.");

            break;  // Exit the while(true)
          }

          Serial.println("Woke from timer — continuing slideshow");
        }
      }

      // sleep but show clock and update every minute
      else if (menu_selected_index == 1)
        enterClockSleep();

      // hibernate and display current file until interrupt
      else if (menu_selected_index == 2) {
        Serial.println("In Hibernate Mode, displaying current image");
        while (true) {
          constexpr gpio_num_t WAKEUP_PIN = GPIO_NUM_26;        // button 1
          gpio_wakeup_enable(WAKEUP_PIN, GPIO_INTR_LOW_LEVEL);  // Trigger wake-up on high level
          esp_sleep_enable_gpio_wakeup();

          if(FuelGauge.percent() < 10)
            break;

          String current_file_location = String(DIRECTORY) + "/" + getCurrentFileName();
          app.currentFile = SD.open(current_file_location, FILE_READ);

          Serial.print("Displaying image: ");
          Serial.println(app.currentFileName);
          displayImageFromBin(app.currentFile);

          Serial.println("Turning off WiFi...");
          stopWifi();
          Serial.println("WiFi Disabled.");

          // sleep
          Serial.println("Now hibernating.");
          delay(500);
          esp_light_sleep_start();

          // After waking
          Serial.println("Woke up!");
          esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

          // handle wakeup on GPIO
          if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
            lastActivityTime = millis();  // Reset timer
            Serial.println("Button Wake Detected — exiting hibernation loop");

            systemState = CLOCK_MODE;

            displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);

            // restart wifi
            Serial.println("Restarting Wifi...");
            restartWiFiAndServer();
            Serial.println("Restarting Wifi completed.");

            break;  // Exit the while(true)
          }
        }
      }
    }
  }
  app.resetButtons();
}

/* ========================== FUNCTION DEFINITION ========================== */
/* ===================== SERVER FUNCTIONS ===================== */
void handle_button_1() {
  Serial.printf("Printing Current File: %s [%d]\n", getCurrentFileName().c_str(), app.currentFileIndex);
  String current_file_location = String(DIRECTORY) + "/" + getCurrentFileName();
  app.currentFile = SD.open(current_file_location, FILE_READ);
  displayImageFromBin(app.currentFile);
}

//Initial page of the server web, list directory and give you the chance of deleting and uploading
void SD_dir() {
  if (app.sdPresent) {
    //Action according to post, download or delete, by MC 2022
    if (server.args() > 0)  //Arguments were received, ignored if there are not arguments
    {
      Serial.println(server.arg(0));

      String Order = server.arg(0);

      if (Order.indexOf("download_") >= 0) {
        Order.remove(0, 9);
        Order = String(DIRECTORY) + "/" + Order;

        SD_file_download(Order);
        Serial.print("'download_' Order (server.arg(0)): ");
        Serial.println(Order);
      }

      if ((server.arg(0)).indexOf("delete_") >= 0) {
        Order.remove(0, 7);
        Order = String(DIRECTORY) + "/" + Order;

        SD_file_delete(Order);
        Serial.print("'delete_' Order (server.arg(0)): ");
        Serial.println(Order);
      }
    }
    File root_server = SD.open(DIRECTORY);

    if (root_server) {
      root_server.rewindDirectory();
      SendHTML_Header();
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th><th>Type File/Dir</th><th>File Size</th><th>Download</th><th>Delete</th></tr>");
      printDirectory(DIRECTORY, 0);
      webpage += F("</table>");
      SendHTML_Content();
    } else {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();  //Stop is needed because no content length was sent
  } else ReportSDNotPresent();
}

//Prints the directory, it is called in void SD_dir()
void printDirectory(const char* dirname, uint8_t levels) {
  File root_server = SD.open(dirname);

  if (!root_server) {
    return;
  }
  if (!root_server.isDirectory()) {
    return;
  }
  File server_file = root_server.openNextFile();

  int i = 0;
  while (server_file) {
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if (server_file.isDirectory()) {
      webpage += "<tr><td>" + String(server_file.isDirectory() ? "Dir" : "File") + "</td><td>" + String(server_file.name()) + "</td><td></td></tr>";
      printDirectory(server_file.name(), levels - 1);
    } else {
      webpage += "<tr><td>" + String(server_file.name()) + "</td>";
      webpage += "<td>" + String(server_file.isDirectory() ? "Dir" : "File") + "</td>";
      int bytes = server_file.size();
      String fsize = "";
      if (bytes < 1024) fsize = String(bytes) + " B";
      else if (bytes < (1024 * 1024)) fsize = String(bytes / 1024.0, 3) + " KB";
      else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
      else fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
      webpage += "<td>" + fsize + "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>");
      webpage += F("<button type='submit' name='download'");
      webpage += F("' value='");
      webpage += "download_" + String(server_file.name());
      webpage += F("'>Download</button>");
      webpage += "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>");
      webpage += F("<button type='submit' name='delete'");
      webpage += F("' value='");
      webpage += "delete_" + String(server_file.name());
      webpage += F("'>Delete</button>");
      webpage += "</td>";
      webpage += "</tr>";
    }
    server_file = root_server.openNextFile();
    i++;
  }
  server_file.close();
}

//Download a file from the SD, it is called in void SD_dir()
void SD_file_download(String filename) {
  lastActivityTime = millis();  // Reset timer

  if (app.sdPresent) {
    File download = SD.open(filename);
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download");
  } else ReportSDNotPresent();
}

//Handles the file upload a file to the SD
File UploadFile;

//Upload a new file to the Filing system
size_t total_bytes_written = 0;

void handleFileUpload() {
  HTTPUpload& uploadfile = server.upload();
  if (uploadfile.status == UPLOAD_FILE_START) {
    // Reset the counter at the start of a new upload
    total_bytes_written = 0; 
    
    String filename = uploadfile.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    filename = String(DIRECTORY) + filename;
    Serial.print("Upload File Name: ");
    Serial.println(filename);
    SD.remove(filename);
    UploadFile = SD.open(filename, FILE_WRITE);
    filename = String();

  } else if (uploadfile.status == UPLOAD_FILE_WRITE) {
    if (UploadFile) {
      size_t bytes_written = UploadFile.write(uploadfile.buf, uploadfile.currentSize);
      // Keep track of the total bytes written
      total_bytes_written += bytes_written; 
    }

  } else if (uploadfile.status == UPLOAD_FILE_END) {
    if (UploadFile) {
      UploadFile.close(); // Finalize the write
      Serial.printf("Upload finished. Total expected: %u, Total written: %u\n", uploadfile.totalSize, total_bytes_written);

      // **** ADD THIS CHECK ****
      if (total_bytes_written == uploadfile.totalSize) {
        // SUCCESS: Send a success response
        Serial.println("File upload successful and size matches.");
        webpage = "";
        append_page_header();
        webpage += F("<h3>File was successfully uploaded</h3>");
        webpage += F("<h2>Uploaded File Name: ");
        webpage += uploadfile.filename + "</h2>";
        webpage += F("<h2>File Size: ");
        webpage += file_size(uploadfile.totalSize) + "</h2><br><br>";
      } else {
        // FAILURE: Send an error response
        Serial.println("!!! File upload failed: Size mismatch.");
        webpage = "";
        append_page_header();
        webpage += F("<h3>ERROR: File upload failed.</h3>");
        webpage += F("<h4>The transfer may have been interrupted. Please try again.</h4>");
      }
      
      webpage += F("<a href='/'>[Back]</a><br><br>");
      append_page_footer();
      server.send(200, "text/html", webpage);
    } else {
      ReportCouldNotCreateFile("upload");
    }
  }
}

String uploadedTimeString;

void handleTimeUpload() {
  lastActivityTime = millis();  // Reset timer

  if (server.hasArg("timeUploadProcess")) {
    uploadedTimeString = server.arg("timeUploadProcess");

    Serial.print("Uploaded time string: ");
    Serial.println(uploadedTimeString);

    // Expected format: "2025-05-27T21:45" or "2025-05-27T21:45:00"
    int y = uploadedTimeString.substring(0, 4).toInt();
    int m = uploadedTimeString.substring(5, 7).toInt();
    int d = uploadedTimeString.substring(8, 10).toInt();
    int h = uploadedTimeString.substring(11, 13).toInt();
    int min = uploadedTimeString.substring(14, 16).toInt();
    int s = (uploadedTimeString.length() >= 19) ? uploadedTimeString.substring(17, 19).toInt() : 0;

    rtc.adjust(DateTime(y, m, d, h, min, s));

    // web response
    webpage = "";
    append_page_header();
    webpage += F("<h2>Time was successfully uploaded</h2>");
    webpage += F("<h3>Time Recieved: ");
    webpage += uploadedTimeString + "</h3>";
    webpage += F("<a href='/'>[Back]</a><br><br>");
    append_page_footer();
    server.send(200, "text/html", webpage);
  } else {
    webpage = "";
    append_page_header();
    webpage += F("<h3>Error: Time was not uploaded</h3>");
    webpage += F("<a href='/'>[Back]</a><br><br>");
    append_page_footer();
    server.send(400, "text/html", webpage);
  }
}

void handleGalleryIntervalUpload() {
  lastActivityTime = millis();  // Reset timer

  if (server.hasArg("galleryIntervalUploadProcess")) {
    String uploadedGalleryIntervalString = server.arg("galleryIntervalUploadProcess");

    Serial.print("Uploaded gallery interval string: ");
    Serial.println(uploadedGalleryIntervalString);

    // handle interval conversion to a integer value multiplying by 60 million
    uploadedGalleryInterval = (uint64_t)(uploadedGalleryIntervalString.toFloat() * 60.0 * 1000000.0);

    // web response
    webpage = "";
    append_page_header();
    webpage += F("<h2>Gallery interval was successfully uploaded</h2>");
    webpage += F("<h3>Interval Recieved: ");
    webpage += uploadedGalleryIntervalString + " minutes</h3>";
    webpage += F("<a href='/'>[Back]</a><br><br>");
    append_page_footer();
    delay(500);
    server.send(200, "text/html", webpage);
    delay(500);

  } else {
    webpage = "";
    append_page_header();
    webpage += F("<h3>Error: Gallery interval was not uploaded</h3>");
    webpage += F("<a href='/'>[Back]</a><br><br>");
    append_page_footer();
    server.send(400, "text/html", webpage);
  }
}

//Delete a file from the SD, it is called in void SD_dir()
void SD_file_delete(String filename) {
  lastActivityTime = millis();  // Reset timer

  Serial.print("Filename: ");
  Serial.println(filename);

  if (app.sdPresent) {
    delay(500);
    Serial.println("SD Is Present");
    SendHTML_Header();
    File dataFile = SD.open(filename, FILE_READ);  //Now read data from SD Card
    delay(500);
    if (dataFile) {
      Serial.println("Datafile Is Present");

      if (SD.remove(filename)) {
        Serial.println(F("File deleted successfully"));

        webpage += "<h3>File '" + filename + "' has been erased</h3>";
        webpage += F("<a href='/'>[Back]</a><br><br>");
      } else {
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
}

//File size conversion
String file_size(int bytes) {
  String fsize = "";
  if (bytes < 1024) fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024)) fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}

/* ===================== DISPLAY FUNCTIONS ===================== */
void display_files() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);  // clear the display
    display.setTextColor(GxEPD_BLACK);
    display.setFont();  // default font
    display.setTextSize(4);
    display.setCursor(60, 20);  // starting position (x,y)
    display.print("Select Files");

    list_dir(SD, DIRECTORY);

    int cursor_position_y = 80;
    display.setTextSize(1);

    int start = getCurrentPage() * MAX_FILES_PER_PAGE;
    int end = min(start + MAX_FILES_PER_PAGE, app.fileCount);

    for (int i = start; i < end; i++) {
      char buffer[MAX_FILENAME_LENGTH + 10];
      sprintf(buffer, "%d: %s", i + 1, app.fileNames[i / MAX_FILES_PER_PAGE][i % MAX_FILES_PER_PAGE].c_str());  // loads filename + index into buffer

      display.setTextSize(2);
      display.setCursor(60, cursor_position_y);
      display.print(buffer);

      cursor_position_y += 30;

      display.setTextSize(1);
      display.setCursor(10, 290);  // starting position (x,y)
      display.print("Selected File: #" + String(app.currentFileIndex + 1) + " " + getCurrentFileName());

      // Optional: Show page info
      display.setCursor(250, 290);
      display.print("Page " + String(getCurrentPage() + 1) + "/" + String(app.totalPages));

      display.setCursor(350, 290);
      display.print("Mode " + String(app.fileMode ? "F" : "P"));
    }
  } while (display.nextPage());
}

void list_dir(fs::FS& fs, const char* dirname) {
  Serial.printf("\nRe-scanning directory: %s\n", dirname);

  File list_root = fs.open(dirname);
  if (!list_root || !list_root.isDirectory()) {
    Serial.println("ERROR: Failed to open directory");
    return;
  }

  int new_file_count = 0; // Use a local counter, starting from 0.

  File file = list_root.openNextFile();
  // Loop ONLY depends on finding a file. No more "app.fileCount".
  while (file) {
    if (!file.isDirectory()) {
      // Prevent writing past the end of your app.fileNames array
      if (new_file_count < (MAX_PAGES * MAX_FILES_PER_PAGE)) {
        // Simplified and safer way to calculate array indices
        int page_index = new_file_count / MAX_FILES_PER_PAGE;
        int file_index = new_file_count % MAX_FILES_PER_PAGE;
        app.fileNames[page_index][file_index] = String(file.name());
        new_file_count++; // Increment count ONLY after successfully storing a file
      } else {
        Serial.println("WARNING: Maximum file limit reached. Some files not listed.");
        break; // Stop if the array is full
      }
    }
    file = list_root.openNextFile(); // Get the next file for the next loop iteration
  }

  // Now, update the global variables with the new, correct counts.
  app.fileCount = new_file_count;
  app.totalPages = (app.fileCount + MAX_FILES_PER_PAGE - 1) / MAX_FILES_PER_PAGE;
  list_root.close();

  Serial.printf("Scan complete. Found and stored %d filenames.\n", app.fileCount);
}

void displayImageFromBin(File& file) {
  uint16_t width = file.read() | (file.read() << 8);
  uint16_t height = file.read() | (file.read() << 8);

  int size = (width * height) / 8;
  Serial.printf("Attempting to allocate %d bytes for a %dx%d image\n", size, width, height);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  uint8_t* buffer = (uint8_t*)malloc(size);
  if (!buffer) {
    Serial.println("Memory alloc failed!");
    file.close(); // Close the file before restarting
    ESP.restart();
    return;
  }

  // VITAL CHECK: Verify that the read was successful
  size_t bytes_read = file.read(buffer, size);
  if (bytes_read != size) {
    Serial.printf("!!! SD Read Error: Expected %d bytes, but only got %lu\n", size, bytes_read);
    free(buffer);     // Clean up the allocated memory
    file.close();     // Close the file
    // Optional: You could display an error message on the e-ink screen here
    return;           // Exit the function to prevent displaying a corrupt image
  }

  // If the code reaches here, the read was successful.
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawXBitmap(0, 0, buffer, width, height, GxEPD_BLACK);
  } while (display.nextPage());

  free(buffer);
}

int getCurrentPage() {
  return app.currentFileIndex / MAX_FILES_PER_PAGE;
}

int getCurrentPageFile() {
  return app.currentFileIndex % MAX_FILES_PER_PAGE;
}

String getCurrentFileName() {
  int page = getCurrentPage();
  int file = getCurrentPageFile();

  Serial.println(app.fileNames[page][file]);

  return app.fileNames[page][file];
}

String formatTime(DateTime now) {
  // Getting each time field in individual variables
  // And adding a leading zero when needed;
  String yearStr = String(now.year(), DEC);
  //String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  String monthStr = months[now.month() - 1];
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC);
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  // Complete time string
  String formattedTime = dayOfWeek + ", " + yearStr + "-" + monthStr + "-" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;

  formattedTimeArray[0] = dayOfWeek + ", " + monthStr + " " + dayStr + ", " + yearStr;
  formattedTimeArray[1] = dayOfWeek;
  formattedTimeArray[2] = (now.hour() == 0) ? "12" : (now.hour() > 12) ? (now.hour() - 12 < 10 ? "0" : "") + String(now.hour() - 12, DEC)
                                                                       : (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC);
  formattedTimeArray[3] = minuteStr;
  formattedTimeArray[4] = (now.hour() >= 12 && now.hour() != 0) ? "PM" : "AM";

  // Print the complete formatted time
  return formattedTime;
}

void displayClock(String time, String dateStr, int batteryPercentage, int minimized) {
  updateClock();
  // Split time into main and suffix (e.g., "12:45 PM")
  String mainTime = time;
  String suffix = "";

  if (time.endsWith("AM") || time.endsWith("PM")) {
    mainTime = time.substring(0, time.length() - 3);
    suffix = time.substring(time.length() - 2);  // "AM" or "PM"
  }

  display.setFont(&FreeSansBold24pt7b);
  display.setTextSize(3);

  // Get bounding box for main time (HH:mm)
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(mainTime, 0, 0, &x1, &y1, &w, &h);

  // Center HH:mm horizontally and vertically
  int x_main = (400 - w) / 2 - 15;
  int y_main = (300 + h) / 2;

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(1);
    display.setCursor(20, 30);
    display.print("Audrey's Clock");

    // Date line (e.g., "Tuesday, May 28, 2025")
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 60);
    display.print(dateStr);

    // sleep indication
    display.setCursor(20, 290);
    display.print(minimized ? "Sleep" : "Active");

    // Footer
    display.setCursor(280, 290);
    display.print("- from Nick :)");

    // Battery rectangle with percentage inside
    int batteryX = 330;
    int batteryY = 10;
    int batteryWidth = 60;
    int batteryHeight = 20;
    int terminalWidth = 4;
    int terminalHeight = 10;

    // Set font and color for battery label
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    String batteryStr = String(batteryPercentage) + "%";

    // Measure width/height of the text
    int16_t tx1, ty1;
    uint16_t tw, th;
    display.getTextBounds(batteryStr, 0, 0, &tx1, &ty1, &tw, &th);

    // Position to the left of the battery icon, vertically centered
    int textX = batteryX - tw - 6;  // 6 px margin
    int textY = batteryY + (batteryHeight + th) / 2 - 2;

    display.setCursor(textX, textY);
    display.print(batteryStr);

    // Draw outer battery shell
    display.drawRect(batteryX, batteryY, batteryWidth, batteryHeight, GxEPD_BLACK);

    // Draw battery terminal
    display.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight - terminalHeight) / 2,
                     terminalWidth, terminalHeight, GxEPD_BLACK);

    // Draw fill bar (minimum width of 2 px for visibility)
    int fillWidth = map(batteryPercentage, 0, 100, 0, batteryWidth - 4);
    if (fillWidth > 0) {
      display.fillRect(batteryX + 2, batteryY + 2, fillWidth, batteryHeight - 4, GxEPD_BLACK);
    }

    // Restore default color for everything else
    display.setTextColor(GxEPD_BLACK);

    // Draw main time (HH:mm)
    display.setFont(&FreeSansBold24pt7b);
    display.setTextSize(3);
    display.setCursor(x_main, y_main);
    display.setTextColor(GxEPD_BLACK);
    display.print(mainTime);

    // Draw suffix (AM/PM) in smaller font in bottom-right
    if (suffix.length() > 0) {
      display.setFont(&FreeSansBold24pt7b);

      int16_t sx1, sy1;
      uint16_t sw, sh;
      display.getTextBounds(suffix, 0, 0, &sx1, &sy1, &sw, &sh);

      int x_suffix = 400 - sw - 30;  // 15px from right edge
      int y_suffix = 300 - 20;       // 15px from bottom

      display.setTextSize(1);
      display.setCursor(x_suffix, y_suffix);
      display.print(suffix);
    }

  } while (display.nextPage());
}

// updates formatted time array with current RTC readings, also updates battery percentage
void updateClock() {
  // Get the current time from the RTC
  now = rtc.now();
  formatTime(now);
  currentTime = formattedTimeArray[2] + ":" + formattedTimeArray[3] + " " + formattedTimeArray[4];

  // get battery percentage
  batteryPercentage = FuelGauge.percent();
}

void waitUntilTopOfMinute() {
  now = rtc.now();
  int seconds = now.second();
  uint64_t delay_microseconds = (59.0 - seconds) * uS_TO_S_FACTOR;

  esp_sleep_enable_timer_wakeup(delay_microseconds);

  // stop wifi
  Serial.println("Turning off WiFi...");
  stopWifi();
  Serial.println("WiFi Disabled.");

  // display
  displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 1);

  // sleep
  Serial.printf("Now sleeping %d seconds to align to top of minute.\n", 59.0 - seconds);
  esp_light_sleep_start();

  // After waking
  Serial.printf("Woke up after %d seconds\n", 59.0 - seconds);
}

void stopWifi() {
  server.stop();
  esp_wifi_stop();
}

void restartWiFiAndServer() {
  esp_wifi_start();
  // 1. Set mode
  WiFi.mode(WIFI_AP);

  // 2. Start SoftAP
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Access Point restarted");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  // 3. Re-register all routes
  server.on("/", SD_dir);
  server.on("/upload", File_Upload);
  server.on(
    "/fupload", HTTP_POST, []() {
      server.send(200);
    },
    handleFileUpload);
  server.on("/timeUpload", Time_Upload);
  server.on("/timeUploadProcess", HTTP_POST, handleTimeUpload);
  server.on("/galleryIntervalUpload", Gallery_Interval_Upload);
  server.on("/galleryIntervalUploadProcess", HTTP_POST, handleGalleryIntervalUpload);

  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302);
  });

  // 4. Restart HTTP server
  server.begin();
  Serial.println("HTTP server restarted");
}

void displayMenu() {
  uint8_t curr_y_pos = 100;
  String menu_elements[MENU_ITEMS] = { "Sleep (Show Gallery)", "Sleep (Show Clock)", "Hibernation (display current file)" };

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

void enterClockSleep() {
  Serial.println("In Sleep Mode, displaying clock");
  while (true) {
    constexpr gpio_num_t WAKEUP_PIN = GPIO_NUM_26;        // button 1
    gpio_wakeup_enable(WAKEUP_PIN, GPIO_INTR_LOW_LEVEL);  // Trigger wake-up on high level

    if(FuelGauge.percent() < 10)
      break;

    esp_sleep_enable_gpio_wakeup();
    esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);

    // do clock here
    waitUntilTopOfMinute();  // waits till top then updates

    displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 1);

    // After waking
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
      lastActivityTime = millis();  // Reset timer
      Serial.println("Button Wake Detected — exiting sleep loop");

      systemState = CLOCK_MODE;

      displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);

      // restart wifi
      Serial.println("Restarting Wifi...");
      restartWiFiAndServer();
      Serial.println("Restarting Wifi completed.");

      break;  // Exit the while(true)
    }
    Serial.println("Woke from timer — continuing clock");
  }
}

void checkLowBattery(){
  // if low, just display low battery
  if (FuelGauge.percent() < 10) {
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);  // clear the display
      display.setTextColor(GxEPD_BLACK);
      display.setFont(&FreeSansBold24pt7b);
      display.setTextSize(1);

      display.setCursor(20, 50);  // starting position (x,y)
      display.print("CHARGE");
      display.setCursor(20, 100);  // starting position (x,y)
      display.print("LOW BATTERY!");

    } while (display.nextPage());

    Serial.println("entering low power mode");

    while (true) {
      constexpr gpio_num_t WAKEUP_PIN = GPIO_NUM_26;        // button 1
      gpio_wakeup_enable(WAKEUP_PIN, GPIO_INTR_LOW_LEVEL);  // Trigger wake-up on high level

      esp_sleep_enable_gpio_wakeup();
      esp_sleep_enable_timer_wakeup(3600ULL * uS_TO_S_FACTOR);

      // Stop server and Wi-Fi
      Serial.println("Turning off WiFi...");
      stopWifi();
      Serial.println("WiFi Disabled.");

      // sleep
      Serial.println("Now Sleeping.");
      delay(500);
      esp_light_sleep_start();

      // After waking
      Serial.println("Woke up!");
      esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

      if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
        lastActivityTime = millis();  // Reset timer
        Serial.println("Button Wake Detected — exiting sleep loop");
        systemState = CLOCK_MODE;

        displayClock(currentTime, formattedTimeArray[0], batteryPercentage, 0);

        // restart wifi
        Serial.println("Restarting Wifi...");
        restartWiFiAndServer();
        Serial.println("Restarting Wifi completed.");

        delay(1000);

        break;  // Exit the while(true)
      }
      else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
      {
        Serial.println("Timer Wake Detected — exiting sleep loop and checking battery");
        if (FuelGauge.percent() < 10)
        {
          // restart wifi
          Serial.println("Restarting Wifi...");
          restartWiFiAndServer();
          Serial.println("Restarting Wifi completed.");

          delay(1000);
          break;
        }
      }
      if (FuelGauge.percent() < 10)
        break;
    }
  }
}
