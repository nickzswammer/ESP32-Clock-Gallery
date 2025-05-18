/*
  Project Title: Audrey Clock Gallery
  File Name:     full_disp_AND_upload.ino
  Author:        Nicholas Zhang
  Date:          May 16, 2025

  Description:
    3 Pushbuttons and gives user interface to select files to display, as well as enabling SD Wifi Uploading

  Notes:
    N/A
*/

/* ========================== LIBRARIES & FILES ========================== */

#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection_new_style.h"

// Wifi Libraries
#include <WiFi.h>              //Built-in
#include <ESP32WebServer.h>    //https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <DNSServer.h>

// SD Libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// CSS file for webpage
#include "WebHelpers.h" //Includes headers of the web and de style file
#include "AppState.h"

// pin definitions
#define CS_SD 2
#define BUTTON_1 26
#define BUTTON_2 25
#define BUTTON_3 33
#define BUTTON_4 32

#define DIRECTORY "/bin_images"
#define DNS_PORT 53

AppState app;

/* ========================== VARIABLES ========================== */

ESP32WebServer server(80);

const byte DNS_PORT_NUM = 53;
DNSServer dnsServer;

/* ========================== SETUP ========================== */

void setup() {
  Serial.begin(115200);
  display.init(115200);
  display.setRotation(0);
  display.setFullWindow();

  /* ===================== WIFI BLOCK ===================== */
  WiFi.softAP("Clock Gallery Access", "moomin123"); //Network and password for the access point genereted by ESP32
  Serial.println("Access Point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  dnsServer.start(DNS_PORT_NUM, "*", WiFi.softAPIP());

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);

  /* ===================== SERVER COMMANDS ===================== */
  server.on("/",         SD_dir);
  server.on("/upload",   File_Upload);
  server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);

  // Catch-all route
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302);
  });


  server.begin();
  
  Serial.println("HTTP server started");

  /* ===================== INIT SD CARD ===================== */
  pinMode(BUTTON_1, INPUT_PULLUP); // button for going to next file in microSD

  Serial.print(F("Initializing SD card... "));
  
  //see if the card is present and can be initialised.
  //Note: Using the ESP32 and SD_Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise may not work
  if (!SD.begin(CS_SD))
  { 
    Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
    app.sdPresent = false; 
  } 
  else
  {
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
  app.currentFileName = app.currentFile.name();
  app.fileNames[0] = app.currentFileName;

  //list_dir(SD, DIRECTORY);

}

void display_files(int show_curr_file)
{
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

    for (int i = 0; i < app.fileCount; i++)
    {
      char buffer[MAX_FILENAME_LENGTH + 10];
      sprintf(buffer, "%d: %s", i+1, app.fileNames[i].c_str()); // loads filename + index into buffer

      display.setCursor(60, cursor_position_y);
      display.print(buffer);

      cursor_position_y += 20;

      if (show_curr_file)
      {
        display.setTextSize(1);
        display.setCursor(120, 290);  // starting position (x,y)
        display.print("Selected File: #" + String(app.currentFileIndex+1) + " " + String(app.fileNames[app.currentFileIndex]));
      }
    }
  } while (display.nextPage());
}

void loop() {
  
  // button states
  app.currButton1 = digitalRead(BUTTON_1);
  app.currButton2 = digitalRead(BUTTON_2);
  app.currButton3 = digitalRead(BUTTON_3);
  app.currButton4 = digitalRead(BUTTON_4);

  server.handleClient(); //Listen for client connections

  app.currentFileName = app.fileNames[app.currentFileIndex];

  // display
  if(app.wasPressed(app.currButton1, app.prevButton1))
  {
    handle_button_1();
  }

    // select prev file
  if(app.wasPressed(app.currButton2, app.prevButton2))
  {
    list_dir(SD, DIRECTORY);

    Serial.println("");

    if(app.fileCount == 0)
    {
      Serial.println("No files found");
      return;
    }
    app.currentFileIndex--;

    if(app.currentFileIndex < 0)
      app.currentFileIndex = app.fileCount - 1;

    Serial.printf("Selected File [%d]: ", app.currentFileIndex);
    Serial.println(app.fileNames[app.currentFileIndex]);
    
    display_files(1); // 1 to indicate to show files
  }

  // select next file
  if(app.wasPressed(app.currButton3, app.prevButton3))
  {
    list_dir(SD, DIRECTORY);

    Serial.println("");

    if(app.fileCount == 0)
    {
      Serial.println("No files found");
      return;
    }
    app.currentFileIndex++;

    if(app.currentFileIndex > app.fileCount - 1)
      app.currentFileIndex = 0;

    Serial.printf("Selected File [%d]: ", app.currentFileIndex);
    Serial.println(app.fileNames[app.currentFileIndex]);
    
    display_files(1); // 1 to indicate to show files
  }

  // show files
  if(app.wasPressed(app.currButton4, app.prevButton4))
  {
    app.displayingFiles = !app.displayingFiles;
    if(app.displayingFiles)
    {
      Serial.println("Showing All Files ...");
      display_files(0);
    }
    else {
      handle_button_1();
    }
  }

  app.resetButtons();

}

/* ========================== FUNCTION DEFINITION ========================== */
/* ===================== SERVER FUNCTIONS ===================== */
void handle_button_1(){
    Serial.printf("Printing Current File: %s[%d]\n", app.currentFileName, app.currentFileIndex);
    String current_file_location = String(DIRECTORY) + "/" + app.fileNames[app.currentFileIndex];
    app.currentFile = SD.open(current_file_location, FILE_READ);
    displayImageFromBin(app.currentFile);
}


//Initial page of the server web, list directory and give you the chance of deleting and uploading
void SD_dir()
{
  if (app.sdPresent) 
  {
    //Action according to post, download or delete, by MC 2022
    if (server.args() > 0 ) //Arguments were received, ignored if there are not arguments
    { 
      Serial.println(server.arg(0));
  
      String Order = server.arg(0);

      
      if (Order.indexOf("download_")>=0)
      {
        Order.remove(0,9);
        Order = String(DIRECTORY) + "/" + Order;

        SD_file_download(Order);
        Serial.print("'download_' Order (server.arg(0)): ");
        Serial.println(Order);
      }
  
      if ((server.arg(0)).indexOf("delete_")>=0)
      {
        Order.remove(0,7);
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
      webpage += F("<tr><th>Name/Type</th><th style='width:20%'>Type File/Dir</th><th>File Size</th></tr>");
      printDirectory(DIRECTORY,0);
      webpage += F("</table>");
      SendHTML_Content();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   //Stop is needed because no content length was sent
  } else ReportSDNotPresent();
}

//Upload a file to the SD
void File_Upload()
{
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:25%' type='file' name='fupload' id = 'fupload' value=''>");
  webpage += F("<button class='buttons' style='width:10%' type='submit'>Upload File</button><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}

//Prints the directory, it is called in void SD_dir() 
void printDirectory(const char * dirname, uint8_t levels)
{
  File root_server = SD.open(dirname);

  if(!root_server){
    return;
  }
  if(!root_server.isDirectory()){
    return;
  }
  File server_file = root_server.openNextFile();

  int i = 0;
  while(server_file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(server_file.isDirectory()){
      webpage += "<tr><td>"+String(server_file.isDirectory()?"Dir":"File")+"</td><td>"+String(server_file.name())+"</td><td></td></tr>";
      printDirectory(server_file.name(), levels-1);
    }
    else
    {
      webpage += "<tr><td>"+String(server_file.name())+"</td>";
      webpage += "<td>"+String(server_file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = server_file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='download'"); 
      webpage += F("' value='"); webpage +="download_"+String(server_file.name()); webpage +=F("'>Download</button>");
      webpage += "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='delete'"); 
      webpage += F("' value='"); webpage +="delete_"+String(server_file.name()); webpage +=F("'>Delete</button>");
      webpage += "</td>";
      webpage += "</tr>";

    }
    server_file = root_server.openNextFile();
    i++;
  }
  server_file.close();
}

//Download a file from the SD, it is called in void SD_dir()
void SD_file_download(String filename)
{
  if (app.sdPresent) 
  { 
    File download = SD.open(filename);
    if (download) 
    {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSDNotPresent();
}

//Handles the file upload a file to the SD
File UploadFile;
//Upload a new file to the Filing system
void handleFileUpload()
{ 
  HTTPUpload& uploadfile = server.upload(); //See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            //For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    filename = String(DIRECTORY) + filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD.remove(filename);                         //Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE);  //Open the file for writing in SD (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          //If the file was successfully created
    {                                    
      UploadFile.close();   //Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br><br>"; 
      webpage += F("<a href='/'>[Back]</a><br><br>");
      append_page_footer();
      server.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}

//Delete a file from the SD, it is called in void SD_dir()
void SD_file_delete(String filename) 
{ 
  Serial.print("Filename: ");
  Serial.println(filename);
  
  if (app.sdPresent) { 
    Serial.println("SD Is Present");
    SendHTML_Header();
    File dataFile = SD.open(filename, FILE_READ); //Now read data from SD Card 
    if (dataFile)
    {
      Serial.println("Datafile Is Present");

      if (SD.remove(filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '"+filename+"' has been erased</h3>"; 
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
      else
      { 
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
String file_size(int bytes)
{
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}

/* ===================== DISPLAY FUNCTIONS ===================== */

void list_dir(fs::FS &fs, const char * dirname) 
{
  Serial.printf("\nListing directory (non-recursive): %s\n", dirname);

  File list_root = fs.open(dirname);
  if (!list_root || !list_root.isDirectory()) 
  {
    Serial.println("Failed to open directory");
    return;
  }

  int i = 0; // iterate through the app.fileNames

  File file = list_root.openNextFile();
  while (file && i < app.fileCount) 
  {
    if (!file.isDirectory()) 
    {
      app.fileNames[i] = String(file.name());
      Serial.printf("FILE: %s\tSIZE: %d\n", file.name(), file.size());
    }

    file = list_root.openNextFile();
    i++;
  }
  app.fileCount = i;
  list_root.close();

  Serial.printf("\nStored %d filenames in app.fileNames[]\n", i);
}

void displayImageFromBin(File& file) 
{
  uint16_t width = file.read() | (file.read() << 8);
  uint16_t height = file.read() | (file.read() << 8);

  int size = (width * height) / 8;
  Serial.printf("Attempting to allocate %d bytes\n", size);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  uint8_t* buffer = (uint8_t*)malloc(size);
  if (!buffer) {
    Serial.println("Memory alloc failed!");
    while(true);
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