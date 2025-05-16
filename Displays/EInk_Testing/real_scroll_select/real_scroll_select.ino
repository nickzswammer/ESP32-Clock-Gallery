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
#define BUTTON_2 25

#define MAX_FILES 20 // maybe change for later? 


// button states
int curr_button_1_state = 1; // high means no press
int prev_button_1_state = 0; // high means no press
int curr_button_2_state = 1; // high means no press
int prev_button_2_state = 0; // high means no press

// File Stores
File root;
File current_file;
String current_file_name = "";
int current_file_names_index = 0;
int file_count = MAX_FILES;

String file_names[MAX_FILES];

void list_dir(fs::FS &fs, const char * dirname);
void select_next_file();


void setup() {
  Serial.begin(115200);
  display.init(115200);

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

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
  current_file = root.openNextFile();
  current_file_name = current_file.name();
  file_names[0] = current_file_name;

}

void loop() {
  // put your main code here, to run repeatedly:
  curr_button_1_state = digitalRead(BUTTON_1);
  curr_button_2_state = digitalRead(BUTTON_2);

  if(curr_button_1_state == LOW && prev_button_1_state == HIGH)
  {
    String current_file_location = "/bin_images/" + file_names[current_file_names_index];
    current_file = SD.open(current_file_location, FILE_READ);
    displayImageFromBin(current_file);
  }

  if(curr_button_2_state == LOW && prev_button_2_state == HIGH)
  {
    list_dir(SD, "/bin_images");

    Serial.println("");

    if(file_count == 0)
    {
      Serial.println("No files found");
      return;
    }
    current_file_names_index++;

    if(current_file_names_index > file_count - 1)
    {
      current_file_names_index = 0;
    }
    Serial.printf("Selected File [%d]: ", current_file_names_index);
    Serial.println(file_names[current_file_names_index]);

  }

  prev_button_1_state = curr_button_1_state;
  prev_button_2_state = curr_button_2_state;

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
    current_file_name = current_file.name();
    Serial.print("\nSelected file: ");
    Serial.println(current_file_name);
  }
}

void list_dir(fs::FS &fs, const char * dirname) {
  Serial.printf("\nListing directory (non-recursive): %s\n", dirname);

  File list_root = fs.open(dirname);
  if (!list_root || !list_root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

  int i = 0; // iterate through the file_names

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

