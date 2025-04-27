/**
 * ESP32 I2S Microphone Test
 * 
 * This sketch tests an I2S microphone connected to ESP32.
 * It reads audio samples and displays statistics in the Serial Monitor.
 * 
 * Connections:
 * - SCK (Bit Clock): GPIO 27
 * - WS (Word Select/LRC): GPIO 26
 * - SD (Data In): GPIO 35
 */

#include <Arduino.h>
#include <driver/i2s.h>

// Pin definitions
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_27  // SCK (Bit Clock)
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_26  // WS (Word Select / LRC) 
#define I2S_MIC_SERIAL_DATA GPIO_NUM_35  // SD (Data In)

// Audio buffer
#define BUFFER_SIZE 1024
int16_t sampleBuffer[BUFFER_SIZE];

// Debug interval
const unsigned long DEBUG_INTERVAL = 1000; // 1 second
unsigned long lastDebugTime = 0;

void setup() {
  // Start Serial connection
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 I2S Microphone Test");
  Serial.println("=========================");
  
  // Initialize I2S for microphone
  if (!setupI2S()) {
    Serial.println("Failed to initialize I2S! Check your connections.");
    while (1); // Stop here if initialization failed
  }
  
  Serial.println("I2S initialized successfully.");
  Serial.println("Starting microphone test...");
  Serial.println("Speak into the microphone to see values change.");
  Serial.println("=========================");
}

void loop() {
  // Read samples from the microphone
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, sampleBuffer, sizeof(sampleBuffer), &bytesRead, 100);
  
  if (result == ESP_OK && bytesRead > 0) {
    int samplesRead = bytesRead / sizeof(int16_t);
    
    // Output debug info periodically
    unsigned long currentTime = millis();
    if (currentTime - lastDebugTime >= DEBUG_INTERVAL) {
      printMicStats(sampleBuffer, samplesRead);
      lastDebugTime = currentTime;
    }
    
    // Draw a simple audio level meter in real time
    drawAudioLevelMeter(sampleBuffer, samplesRead);
  }
  else {
    if (result != ESP_OK) {
      Serial.print("Error reading I2S data: ");
      Serial.println(result);
      delay(1000);
    }
  }
}

// Setup I2S for microphone input
bool setupI2S() {
  // I2S configuration for microphone
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  // I2S pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,     // Serial clock pin
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,  // Word select pin
    .data_out_num = I2S_PIN_NO_CHANGE,      // No output pin needed
    .data_in_num = I2S_MIC_SERIAL_DATA      // Data in pin
  };
  
  // First uninstall the driver if installed
  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
  
  // Install and configure I2S driver
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.print("Error installing I2S driver: ");
    Serial.println(result);
    return false;
  }
  
  // Set the I2S pins
  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.print("Error setting I2S pins: ");
    Serial.println(result);
    return false;
  }
  
  // Clear the DMA buffer
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  return true;
}

// Print microphone statistics
void printMicStats(int16_t* buffer, int bufferSize) {
  int16_t minSample = 32767;
  int16_t maxSample = -32768;
  float sum = 0.0;
  int nonZeroCount = 0;
  float sumOfSquares = 0.0;
  
  for (int i = 0; i < bufferSize; i++) {
    int16_t sample = buffer[i];
    
    if (sample < minSample) minSample = sample;
    if (sample > maxSample) maxSample = sample;
    
    sum += sample;
    sumOfSquares += sample * sample;
    
    if (sample != 0) nonZeroCount++;
  }
  
  float mean = sum / bufferSize;
  float rms = sqrt(sumOfSquares / bufferSize);
  
  Serial.println("\n--- Microphone Statistics ---");
  Serial.print("Samples read: ");
  Serial.println(bufferSize);
  Serial.print("Non-zero samples: ");
  Serial.print(nonZeroCount);
  Serial.print(" (");
  Serial.print((float)nonZeroCount / bufferSize * 100.0);
  Serial.println("%)");
  Serial.print("Min value: ");
  Serial.println(minSample);
  Serial.print("Max value: ");
  Serial.println(maxSample);
  Serial.print("Range: ");
  Serial.println(maxSample - minSample);
  Serial.print("Mean value: ");
  Serial.println(mean);
  Serial.print("RMS value: ");
  Serial.println(rms);
  
  // If the microphone is working, we should see significant values
  if (maxSample - minSample < 100) {
    Serial.println("WARNING: Very low audio range detected!");
    Serial.println("Possible issues:");
    Serial.println("1. Microphone not connected properly");
    Serial.println("2. Microphone power issue");
    Serial.println("3. Incorrect pin configuration");
  } 
  else if (nonZeroCount < bufferSize * 0.1) {
    Serial.println("WARNING: Few non-zero samples!");
    Serial.println("Microphone may not be capturing audio correctly.");
  }
  else {
    Serial.println("Microphone appears to be working");
  }
  Serial.println("-----------------------------");
}

// Draw a simple real-time audio level meter in the serial console
void drawAudioLevelMeter(int16_t* buffer, int bufferSize) {
  // Calculate RMS (Root Mean Square) as audio level
  float sumOfSquares = 0.0;
  for (int i = 0; i < bufferSize; i++) {
    sumOfSquares += buffer[i] * buffer[i];
  }
  float rms = sqrt(sumOfSquares / bufferSize);
  
  // Normalize to 0-50 range for display
  int meterLevel = map(constrain(rms, 0, 5000), 0, 5000, 0, 50);
  
  // Draw meter every 100ms
  static unsigned long lastMeterUpdate = 0;
  if (millis() - lastMeterUpdate >= 100) {
    Serial.print("\rAudio Level: [");
    for (int i = 0; i < 50; i++) {
      if (i < meterLevel) {
        Serial.print("#");
      } else {
        Serial.print(" ");
      }
    }
    Serial.print("] ");
    Serial.print(rms);
    Serial.print("   "); // Extra spaces to clear any previous characters
    
    lastMeterUpdate = millis();
  }
}
