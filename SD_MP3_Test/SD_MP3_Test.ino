#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <SD.h>
#include <SPI.h>

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting setup...");

  // Init SD card
  if (!SD.begin(5)) {
    Serial.println("SD card failed!");
    while (true);
  }
  Serial.println("SD card initialized.");

  // Check if file exists
  if (!SD.exists("/song.mp3")) {
    Serial.println("MP3 file not found!");
    while (true);
  }
  Serial.println("Found /song.mp3");

  // Setup I2S output
  out = new AudioOutputI2S();
  out->SetPinout(26, 25, 22);
  out->SetGain(0.5);

  file = new AudioFileSourceSD("/song.mp3");
  mp3 = new AudioGeneratorMP3();

  if (mp3->begin(file, out)) {
    Serial.println("MP3 playback started");
  } else {
    Serial.println("Failed to begin MP3 playback");
    while (true);
  }
}

void loop() {
  if (mp3->isRunning()) {
    mp3->loop();
  } else {
    Serial.println("Playback finished.");
    delay(2000);
    while (true);
  }
}
