#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <FS.h>

#define MAX_FILES 20
#define MAX_FILENAME_LENGTH 200

struct AppState {
    // Button states
    int currButton1 = 1, prevButton1 = 0;
    int currButton2 = 1, prevButton2 = 0;
    int currButton3 = 1, prevButton3 = 0;
    int currButton4 = 1, prevButton4 = 0;
  
    // Display control
    bool displayingFiles = false;
  
    // SD state
    bool sdPresent = false;
    File root;
    File currentFile;
    String currentFileName = "";
  
    // File indexing
    int currentFileIndex = 0;
    int fileCount = MAX_FILES;
    String fileNames[MAX_FILES];
  
    void resetButtons() {
      prevButton1 = currButton1;
      prevButton2 = currButton2;
      prevButton3 = currButton3;
      prevButton4 = currButton4;
    }
  
    bool wasPressed(int curr, int prev) {
      return (curr == LOW && prev == HIGH);
    }
  };

extern AppState app;

#endif
