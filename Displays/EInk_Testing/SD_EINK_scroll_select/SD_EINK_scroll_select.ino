/*
  Project Title: Audrey Clock Gallery
  File Name:     SD_EINK_scroll_select.ino
  Author:        Nicholas Zhang
  Date:          May 12, 2025

  Description:
    Uses 4 pushbuttons to select image from SD card directory and scroll through

  Notes:
    N/A
*/

#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection_new_style.h"

// SD Libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// pin definitions
#define CS_SD 2
#define BUTTON_1 26

// button states
int curr_button_1_state = 1; // high means no press
int prev_button_1_state = 0; // high means no press

// File Stores
File root;
File current_file;
String selected_file_name = "";

void list_dir(fs::FS &fs, const char * dirname);
void select_next_file();


void setup() {
  Serial.begin(115200);
  display.init(115200);

  pinMode(BUTTON_1, INPUT_PULLUP);

  if(!SD.begin(CS_SD))  // change this to your SD_CS pin
  {
    Serial.println("SD Card Failed to Initialize");
    while(true);
  }

  root = SD.open("/bin_images");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  curr_button_1_state = digitalRead(BUTTON_1);

  // display image
  Serial.println("Displaying: " + selected_file_name);

  if(curr_button_1_state == LOW && prev_button_1_state == HIGH)
  {
    list_dir(SD, "/bin_images");
    select_next_file();
    displayImageFromBin(current_file);
  }
  prev_button_1_state = curr_button_1_state;

}

// Recursive function to list directories and files
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

void list_dir(fs::FS &fs, const char * dirname) {
  Serial.printf("Listing directory (non-recursive): %s\n", dirname);

  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) 
    {
      Serial.print("DIR : ");
      Serial.println(file.name());
      // Do NOT recurse into subdirectory
    } 
    else 
    {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }

    file = root.openNextFile();
  }
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

