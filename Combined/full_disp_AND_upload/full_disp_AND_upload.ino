/*
  Project Title: Audrey Clock Gallery
  File Name:     full_disp_AND_upload.ino
  Author:        Nicholas Zhang
  Date:          May 16, 2025

  Description:
  43 Pushbuttons and gives user interface to select files to display, as well as enabling SD Wifi Uploading
*/

/* ========================== LIBRARIES & FILES ========================== */

#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection_new_style.h"

// Wifi Libraries
#include <WiFi.h>            //Built-in
#include <ESP32WebServer.h>  //https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder

// SD Libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// RTC Library Include
#include "RTClib.h"

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

#define DIRECTORY "/bin_images"

AppState app;

/* ========================== VARIABLES ========================== */

ESP32WebServer server(80);
RTC_DS3231 rtc;

String formattedTimeArray[5] = {"2025-05-27", "Tuesday", "11", "59", "AM"};

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/* ========================== SETUP ========================== */

void setup() {
  Serial.begin(115200);
  display.init(115200);
  display.setRotation(0);
  display.setFullWindow();

  /* ===================== WIFI BLOCK ===================== */
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);  //Network and password for the access point gemerated by ESP32
  Serial.println("Access Point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);

  /* ===================== SERVER COMMANDS ===================== */
  server.on("/", SD_dir);
  server.on("/upload", File_Upload);
  server.on(
    "/fupload", HTTP_POST, []() {
      server.send(200);
    },
    handleFileUpload);
  server.on("/timeUpload", Time_Upload);
  server.on("/timeUploadProcess", HTTP_POST, handleTimeUpload);

  // Catch-all route
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302);
  });

  server.begin();
  Serial.println("HTTP server started");

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

  // RTC Module
  if (! rtc.begin()) {
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

  Serial.println("SETUP EXECUTED!");
}

void loop() {
  // button states
  app.currButton1 = digitalRead(BUTTON_1);
  app.currButton2 = digitalRead(BUTTON_2);
  app.currButton3 = digitalRead(BUTTON_3);
  app.currButton4 = digitalRead(BUTTON_4);

  // Get the current time from the RTC
  DateTime now = rtc.now();

  Serial.printf("\nCurrent Formatted Time: %s\n", formatTime(now).c_str());
  Serial.printf("\nCurrent Time: %s:%s %s\n", formattedTimeArray[2], formattedTimeArray[3], formattedTimeArray[4]);

  //Listen for client connections
  server.handleClient();

  // set file name
  app.currentFileName = getCurrentFileName();

  // display
  if (app.wasPressed(app.currButton1, app.prevButton1)) {
    handle_button_1();
  }

  // select prev file
  if (app.wasPressed(app.currButton2, app.prevButton2)) {
    list_dir(SD, DIRECTORY);

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
      int currentPage = getCurrentPage();
      currentPage = (currentPage - 1 + app.totalPages) % app.totalPages;

      app.currentFileIndex = currentPage * MAX_FILES_PER_PAGE;

      Serial.printf("Selected Page Number: #%d\n", currentPage + 1);
    }
    Serial.println(String(app.currentFileIndex + 1) + " " + getCurrentFileName());
    display_files();
  }

  // select next file
  if (app.wasPressed(app.currButton3, app.prevButton3)) {
    list_dir(SD, DIRECTORY);

    Serial.println("");

    if (app.fileCount == 0) {
      Serial.println("No files found");
      return;
    }
    if (app.fileMode) {
      app.currentFileIndex++;

      if (app.currentFileIndex > app.fileCount - 1)
        app.currentFileIndex = 0;

      Serial.printf("Selected File Number: #[%d]: ", app.currentFileIndex);
    } else {
      // PAGE SELECT MODE: move to next page
      int currentPage = getCurrentPage();
      currentPage = (currentPage + 1 + app.totalPages) % app.totalPages;
      app.currentFileIndex = currentPage * MAX_FILES_PER_PAGE;

      Serial.printf("Selected Page Number: #%d\n", currentPage + 1);
    }
    Serial.println(String(app.currentFileIndex + 1) + " " + getCurrentFileName());

    display_files();
  }

  // show files
  if (app.wasPressed(app.currButton4, app.prevButton4)) {
    app.fileMode = !app.fileMode;
    if (app.fileMode) {
      Serial.println("Changed to File Select Mode");
    } else {
      Serial.println("Changed to Page Select Mode");
    }
    display_files();
  }
  app.resetButtons();
}

/* ========================== FUNCTION DEFINITION ========================== */
/* ===================== SERVER FUNCTIONS ===================== */
void handle_button_1() {
  Serial.printf("Printing Current File: %s [%d]\n", getCurrentFileName(), app.currentFileIndex);
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
void handleFileUpload() {
  HTTPUpload& uploadfile = server.upload();  //See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                             //For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if (uploadfile.status == UPLOAD_FILE_START) {
    String filename = uploadfile.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    filename = String(DIRECTORY) + filename;
    Serial.print("Upload File Name: ");
    Serial.println(filename);
    SD.remove(filename);                         //Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE);  //Open the file for writing in SD (create it, if doesn't exist)
    filename = String();
  } else if (uploadfile.status == UPLOAD_FILE_WRITE) {
    if (UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize);  // Write the received bytes to the file
  } else if (uploadfile.status == UPLOAD_FILE_END) {
    if (UploadFile)  //If the file was successfully created
    {
      UploadFile.close();  //Close the file again
      Serial.print("Upload Size: ");
      Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>");
      webpage += F("<h2>Uploaded File Name: ");
      webpage += uploadfile.filename + "</h2>";
      webpage += F("<h2>File Size: ");
      webpage += file_size(uploadfile.totalSize) + "</h2><br><br>";
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


//Delete a file from the SD, it is called in void SD_dir()
void SD_file_delete(String filename) {
  Serial.print("Filename: ");
  Serial.println(filename);

  if (app.sdPresent) {
    Serial.println("SD Is Present");
    SendHTML_Header();
    File dataFile = SD.open(filename, FILE_READ);  //Now read data from SD Card
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
  Serial.printf("\nListing directory (non-recursive): %s\n", dirname);

  File list_root = fs.open(dirname);
  if (!list_root || !list_root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

  int page_index = 0;       // iterate through the pages
  int file_index = 0;       // iterate thru files on one page
  int loop_file_count = 0;  // file count

  File file = list_root.openNextFile();
  while (file && loop_file_count < app.fileCount) {
    if (!file.isDirectory()) {
      if (loop_file_count != 0 && loop_file_count % MAX_FILES_PER_PAGE == 0) {
        page_index++;
        file_index = 0;
      } else if (loop_file_count != 0) {
        file_index++;
      }
      app.fileNames[page_index][file_index] = String(file.name());
      Serial.printf("FILE: %s\tSIZE: %d\n", file.name(), file.size());
    }

    file = list_root.openNextFile();
    loop_file_count++;
  }
  app.fileCount = loop_file_count;
  app.totalPages = (app.fileCount + MAX_FILES_PER_PAGE - 1) / MAX_FILES_PER_PAGE;
  list_root.close();

  Serial.printf("\nStored %d filenames in app.fileNames[]\n", loop_file_count);
}

void displayImageFromBin(File& file) {
  uint16_t width = file.read() | (file.read() << 8);
  uint16_t height = file.read() | (file.read() << 8);

  int size = (width * height) / 8;
  Serial.printf("Attempting to allocate %d bytes\n", size);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  uint8_t* buffer = (uint8_t*)malloc(size);
  if (!buffer) {
    Serial.println("Memory alloc failed!");
    while (true)
      ;
    return;
  }

  file.read(buffer, size);

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
  return app.fileNames[page][file];
}

String formatTime(DateTime now){
  // Getting each time field in individual variables
  // And adding a leading zero when needed;
  String yearStr = String(now.year(), DEC);
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC); 
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  // Complete time string
  String formattedTime = dayOfWeek + ", " + yearStr + "-" + monthStr + "-" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;

  formattedTimeArray[0] = yearStr + "-" + monthStr + "-" + dayStr;
  formattedTimeArray[1] = dayOfWeek;
  formattedTimeArray[2] = (now.hour() == 0) ? "12" : (now.hour() > 12) ? String(now.hour() - 12, DEC) : String(now.hour(), DEC);
  formattedTimeArray[3] = minuteStr;
  formattedTimeArray[4] = (now.hour() >= 12 && now.hour() != 0) ? "PM" : "AM";

  // Print the complete formatted time
  return formattedTime;
}