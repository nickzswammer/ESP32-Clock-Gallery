#include <SPI.h>
#include <SD.h>

void setup() {
  Serial.begin(115200);
  if (!SD.begin(5)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD init success!");

  File file = SD.open("/test.txt", FILE_WRITE);
  if (file) {
    file.println("Hello from ESP32!");
    file.close();
    Serial.println("Write OK");
  } else {
    Serial.println("Failed to open file");
  }
}

void loop() {
}
