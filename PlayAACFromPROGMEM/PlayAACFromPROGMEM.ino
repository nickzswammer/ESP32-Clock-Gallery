#include <Arduino.h>
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "sampleaac.h"

AudioFileSourcePROGMEM *in;
AudioGeneratorAAC *aac;
AudioOutputI2S *out;

void setup()
{
  Serial.begin(115200);

  audioLogger = &Serial;
  in = new AudioFileSourcePROGMEM(sampleaac, sizeof(sampleaac));
  
  if (!in) {
    Serial.println("Failed to initialize AAC file source!");
    return;
  }

  aac = new AudioGeneratorAAC();

  if (!aac) {
    Serial.println("Failed to initialize AAC decoder!");
    return;
  }

  out = new AudioOutputI2S();

  if (!out) {
    Serial.println("Failed to initialize I2S output!");
    return;
  }

  out -> SetGain(0.125);
  out->SetPinout(26, 25, 22); // Example: BCLK, LRC, DOUT
  out->begin();

  if (aac->begin(in, out)) {
    Serial.println("AAC started successfully!");
  } else {
    Serial.println("AAC failed to start!");
  }
}

void loop()
{
  if (aac->isRunning()) {
    Serial.println("AAC is playing...");
    aac->loop();
  } else {
    Serial.println("AAC stopped, checking file...");
    Serial.printf("Bytes available: %d\n", in->getSize());

    aac->stop();
    out->begin();  // Reinitialize I2S before retrying

    Serial.printf("AAC done\n");
    delay(1000);
  }
}
