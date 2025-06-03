#include <driver/i2s.h>

static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number

// Config I2S

static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = 0,
    .tx_desc_auto_clear = true,
    .fixed_mclk = -1
};

static const i2s_pin_config_t pin_config = {
  .bck_io_num = 27,
  .ws_io_num = 26,
  .data_out_num = 25,
  .data_in_num = I2S_PIN_NO_CHANGE
};



void setup() {
  // put your setup code here, to run once:
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  i2s_set_pin(i2s_num, &pin_config);
}

void loop() {
  // put your main code here, to run repeatedly:
    // This code generates a square wave, the frequency is changed by changing the WaveLength var below, the higher the
  // number the lower the frequency. This var simply acts as a timer for how long we stay high (peak) and low (trough)

  static const uint16_t Volume=0x3ff;             // Highest value the peak will get to, bigger gives louder vol, but  
                                                  // too high can lead to distortion on cheap or small speakers
  static const int16_t Peak=Volume;               // Max height/peak  
  static const int16_t Trough=-Volume;            // Max low/trough
  
  static int16_t OutputValue=Peak;                // Value to send to each channel (left/right), start off at the peak
  static const uint16_t WaveLength=100;           // Bigger =longer wavelength and higher frequency
  static uint16_t TimeAtPeakOrTrough=WaveLength;  // Var to count down how long we hold at either peak or trough
                                                  // (high/low)
  uint32_t Value32Bit;                            // This holds the value we actually send to the I2S buffers, it basically
                                                  // will hold the "OutputValue" but combined for both left/right channels
                                                  
  size_t BytesWritten;                            // Returned by the I2S write routine, we are not interested in it but
                                                  // must supply it as a param to the routine.
                                                    
  if(TimeAtPeakOrTrough==0)                       // If 0 then we're ready to switch to a peak (high) or trough (low)
  {
    if(OutputValue==Peak)                         // If we were a peak (high), switch to trough (low) value
        OutputValue=Trough;
    else                                          // else we were a trough (low) switch to peak (high)
        OutputValue=Peak;
    TimeAtPeakOrTrough=WaveLength;                // Reset this counter back to max, ready to countdown again
  }
  TimeAtPeakOrTrough--;                           // Decrement this counter that controls wavelenght/ frequency

  // This next line just creates the 32 bit word we send to the I2S interface/buffers. 16 bits for the righ channel
  // and 16 bits for the left, as we are sending the same wave to both channels we just combine them using some
  // bit shifting and masking. If you're not sure what's happening here then look up bit shifting/ manipulation
  // on a C programming website.
  
  Value32Bit = (OutputValue<<16) | (OutputValue & 0xffff); // Output same value on both left and right channels

  // Output the combined 32bit value, will not return from this routine until written (blocks your code). Look
  // up this function for further info  
  i2s_write(i2s_num,&Value32Bit,4,&BytesWritten,portMAX_DELAY ); 
}

