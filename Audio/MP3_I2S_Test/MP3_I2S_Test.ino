#include <driver/i2s.h>

#define SAMPLE_RATE     44100
#define BCLK_PIN        26
#define LRC_PIN         25
#define DIN_PIN         22

void setup() {
  Serial.begin(115200);
  Serial.println("Starting square wave test");

  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = BCLK_PIN,
    .ws_io_num = LRC_PIN,
    .data_out_num = DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

void loop() {
  int16_t sampleHigh = 16000;
  int16_t sampleLow = -16000;
  int16_t squareWave[128];

  // Make square wave pattern
  for (int i = 0; i < 64; i++) {
    squareWave[i * 2] = sampleHigh;    // Left
    squareWave[i * 2 + 1] = sampleHigh;  // Right
  }
  for (int i = 64; i < 128; i++) {
    squareWave[i * 2] = sampleLow;
    squareWave[i * 2 + 1] = sampleLow;
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0, squareWave, sizeof(squareWave), &bytesWritten, portMAX_DELAY);
}
