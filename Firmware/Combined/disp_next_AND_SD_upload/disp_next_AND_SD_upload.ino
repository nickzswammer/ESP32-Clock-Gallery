/*
  Project Title: Audrey Clock Gallery
  File Name:     disp_next_AND_SD_upload.ino
  Author:        Nicholas Zhang
  Date:          May 14, 2025

  Description:
    Uses pushbutton to select image from SD card directory and scroll through, as well as enable WiFi uploading

  Notes:
    N/A
*/

/* ========================== LIBRARIES & FILES ========================== */

// GxEPD2 400x300 Board Selection
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

// pin definitions
#define CS_SD 2
#define BUTTON_1 26
#define DIRECTORY "/bin_images"
#define DNS_PORT 53
#define MAX_FILES 20

/* ========================== VARIBLES ========================== */
// create webserver object (listens for incoming connections on specified port HTTP requests)
ESP32WebServer server(80);

const byte DNS_PORT_NUM = 53;
DNSServer dnsServer;

// button states
int curr_button_1_state = 1; // high means no press
int prev_button_1_state = 0; // high means no press

bool SD_present = false; //Controls if the SD card is present or not

// File Stores
File root;
File current_file;
String selected_file_name = "";
int file_count = MAX_FILES;
String file_names[MAX_FILES];

/* ========================== SETUP ========================== */

void setup() {
  Serial.begin(115200);
  display.init(115200);

  /* ===================== WIFI BLOCK ===================== */
  WiFi.softAP("Clock Gallery Access", "moomin123"); //Network and password for the access point genereted by ESP32
  Serial.println("Access Point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  dnsServer.start(DNS_PORT_NUM, "*", WiFi.softAPIP());

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

  Serial.print(F("Initializing SD card..."));
  
  //see if the card is present and can be initialised.
  //Note: Using the ESP32 and SD_Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise may not work
  if (!SD.begin(CS_SD))
  { 
    Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
    SD_present = false; 
  } 
  else
  {
    Serial.println(F("Card initialised... file access enabled..."));
    SD_present = true; 
  }

  // starts off directory inside /bin_images
  root = SD.open(DIRECTORY);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

}

/* ========================== LOOP ========================== */

void loop() {
  server.handleClient(); //Listen for client connections

  curr_button_1_state = digitalRead(BUTTON_1);

  // display image

  if(curr_button_1_state == LOW && prev_button_1_state == HIGH)
  {
    update_file_names(SD, "/bin_images");
    select_next_file();
    displayImageFromBin(current_file);
    Serial.println("Displaying: " + selected_file_name);
  }
  prev_button_1_state = curr_button_1_state;

}

/* ========================== FUNCTION DEFINITION ========================== */

/* ===================== SERVER FUNCTIONS ===================== */

//Initial page of the server web, list directory and give you the chance of deleting and uploading
void SD_dir()
{
  if (SD_present) 
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
  File file = root_server.openNextFile();

  int i = 0;
  while(file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(file.isDirectory()){
      webpage += "<tr><td>"+String(file.isDirectory()?"Dir":"File")+"</td><td>"+String(file.name())+"</td><td></td></tr>";
      printDirectory(file.name(), levels-1);
    }
    else
    {
      webpage += "<tr><td>"+String(file.name())+"</td>";
      webpage += "<td>"+String(file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='download'"); 
      webpage += F("' value='"); webpage +="download_"+String(file.name()); webpage +=F("'>Download</button>");
      webpage += "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='delete'"); 
      webpage += F("' value='"); webpage +="delete_"+String(file.name()); webpage +=F("'>Delete</button>");
      webpage += "</td>";
      webpage += "</tr>";

    }
    file = root_server.openNextFile();
    i++;
  }
  file.close();
}

//Download a file from the SD, it is called in void SD_dir()
void SD_file_download(String filename)
{
  if (SD_present) 
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
  
  if (SD_present) { 
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

/* ===================== SD FUNCTIONS ===================== */

// function to list directories and files (only top level)
void select_next_file() {

  // close file if open already
  if (current_file)
  {
    current_file.close();
  }

  // move to next file
  current_file = root.openNextFile();

  // If we reach the end of the folder, rewind
  if (!current_file) {
    root.rewindDirectory();
    current_file = root.openNextFile();
  }

  // Skip non-bin files
  while (current_file && !String(current_file.name()).endsWith(".bin")) {
    current_file = root.openNextFile();

    // if reaches end while in loop, go back to the beginning of directory
    if (!current_file) {
      root.rewindDirectory();
      current_file = root.openNextFile();
    }
  }

  // output purposes
  if (current_file) {
    selected_file_name = current_file.name();
    Serial.print("\nSelected file: ");
    Serial.println(selected_file_name);
  }
}

void update_file_names(fs::FS &fs, const char * dirname) {
  Serial.printf("Listing directory (non-recursive): %s\n", dirname);

  File list_root = fs.open(dirname);
  if (!list_root || !list_root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

  int i = 0;

  File file = list_root.openNextFile();
  while (file && i < file_count) {
    if (!file.isDirectory()) 
    {
      file_names[i] = String(file.name());
      Serial.printf("FILE: %s\tSIZE: %d\n", file.name(), file.size());
    }

    file = list_root.openNextFile();
    i++;
  }
  file_count = i;
  list_root.close();

  Serial.printf("\nStored %d filenames in file_names[]\n", i);
}

/* ===================== DISPLAY FUNCTIONS ===================== */

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
    while(true);
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

/* ===================== HTML HELPER FUNCTIONS ===================== */

//SendHTML_Header
void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); //Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Content
void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Stop
void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop(); //Stop is needed because no content length was sent
}

//ReportSDNotPresent
void ReportSDNotPresent()
{
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportFileNotPresent
void ReportFileNotPresent(String target)
{
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportCouldNotCreateFile
void ReportCouldNotCreateFile(String target)
{
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

