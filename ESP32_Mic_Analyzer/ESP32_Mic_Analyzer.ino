#include <driver/i2s.h>

// I2S mic pins (INMP441)
#define I2S_WS   25
#define I2S_SCK  32
#define I2S_SD   33
#define I2S_PORT I2S_NUM_0

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 256

void setup() {
  Serial.begin(2000000); // High baud rate for audio streaming

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

void loop() {
  int16_t buffer[BUFFER_SIZE];
  size_t bytesRead = 0;
  i2s_read(I2S_PORT, (void*)buffer, BUFFER_SIZE * sizeof(int16_t), &bytesRead, portMAX_DELAY);
  if (bytesRead == BUFFER_SIZE * sizeof(int16_t)) {
    Serial.write((uint8_t*)buffer, bytesRead);
  }
}
