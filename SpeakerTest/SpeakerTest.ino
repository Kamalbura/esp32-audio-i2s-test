/**
 * ESP32 I2S Speaker Test
 * 
 * This sketch tests an I2S speaker/amplifier connected to ESP32.
 * It generates various test tones to verify the speaker is working.
 * 
 * Connections:
 * - BCK (Bit Clock): GPIO 13
 * - WS (Word Select/LRC): GPIO 25
 * - DATA: GPIO 22
 */

#include <Arduino.h>
#include <driver/i2s.h>

// Pin definitions for I2S speaker output
#define I2S_SPEAKER_SERIAL_CLOCK GPIO_NUM_13  // BCK (Bit Clock)
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK GPIO_NUM_25  // WS (Word Select)
#define I2S_SPEAKER_SERIAL_DATA GPIO_NUM_22  // DATA

// Audio buffer and sample rate
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024
int16_t sampleBuffer[BUFFER_SIZE];

// Test pattern state
enum TestPattern {
  SINE_WAVE,
  SQUARE_WAVE,
  SAWTOOTH_WAVE,
  WHITE_NOISE,
  SWEEP,
  MICROPHONE // New: Use microphone input as audio source
};

// Current test parameters
TestPattern currentPattern = SINE_WAVE;
float currentFrequency = 440.0; // A4 note
float sweepMin = 100.0;
float sweepMax = 3000.0;
float sweepRate = 0.5; // Hz - complete sweep cycle every 2 seconds
int testDuration = 3000; // milliseconds per test
unsigned long lastPatternChange = 0;

// Microphone settings (for when we use microphone input)
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_27  // SCK (Bit Clock)
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_26  // WS (Word Select / LRC) 
#define I2S_MIC_SERIAL_DATA GPIO_NUM_35  // SD (Data In)
bool micInitialized = false;
bool streamingToSerial = false; // Flag to control streaming to Python

// Serial streaming settings
#define STREAM_MAGIC_HEADER 0xAA55
#define STREAM_PACKET_SIZE 256

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 I2S Speaker Test");
  Serial.println("======================");
  
  // Initialize I2S for speaker
  if (!setupI2S()) {
    Serial.println("Failed to initialize I2S! Check your connections.");
    while (1); // Stop here if initialization failed
  }
  
  Serial.println("I2S initialized successfully.");
  Serial.println("Starting speaker test sequence...");
  Serial.println("You should hear different audio test patterns.");
  printCurrentPattern();
  
  lastPatternChange = millis();
}

void loop() {
  // Check if it's time to change the pattern (not when streaming)
  if (!streamingToSerial && millis() - lastPatternChange > testDuration) {
    // Switch to next test pattern
    currentPattern = (TestPattern)((currentPattern + 1) % 6); // Now including MICROPHONE
    lastPatternChange = millis();
    printCurrentPattern();
  }
  
  // Generate/capture the audio samples based on current pattern
  if (currentPattern == MICROPHONE) {
    if (!micInitialized) {
      setupMicrophone();
      micInitialized = true;
    }
    captureMicrophoneData();
  } else {
    if (micInitialized) {
      cleanupMicrophone();
      setupI2S(); // Reinitialize speaker I2S
      micInitialized = false;
    }
    generateAudio();
  }
  
  // If not streaming to serial, send the audio to the speaker
  if (!streamingToSerial) {
    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_0, sampleBuffer, sizeof(sampleBuffer), &bytesWritten, 100);
  }
  
  // If streaming to serial, send audio data with header
  if (streamingToSerial) {
    sendAudioToSerial();
  }
  
  // Handle serial commands if available
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
}

// Setup I2S for microphone input
bool setupMicrophone() {
  // Clean up speaker I2S first
  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
  
  // I2S configuration for microphone
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
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
  
  // I2S pin configuration for microphone
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
  };
  
  // Install and configure I2S driver for microphone
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.print("Error installing I2S driver for microphone: ");
    Serial.println(result);
    return false;
  }
  
  // Set the I2S pins for microphone
  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.print("Error setting I2S pins for microphone: ");
    Serial.println(result);
    return false;
  }
  
  // Clear the DMA buffer
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  Serial.println("Microphone I2S initialized");
  return true;
}

// Cleanup microphone I2S
void cleanupMicrophone() {
  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
}

// Capture data from microphone into sample buffer
void captureMicrophoneData() {
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, sampleBuffer, sizeof(sampleBuffer), &bytesRead, 100);
  
  if (result != ESP_OK) {
    Serial.print("Error reading from microphone: ");
    Serial.println(result);
  }
}

// Send audio data to serial port in binary format for Python
void sendAudioToSerial() {
  // Send magic header to identify start of packet
  Serial.write((STREAM_MAGIC_HEADER >> 8) & 0xFF);
  Serial.write(STREAM_MAGIC_HEADER & 0xFF);
  
  // Send packet size (how many samples)
  Serial.write((STREAM_PACKET_SIZE >> 8) & 0xFF);
  Serial.write(STREAM_PACKET_SIZE & 0xFF);
  
  // Send the actual audio data
  for (int i = 0; i < STREAM_PACKET_SIZE; i++) {
    int index = i % BUFFER_SIZE; // Loop through buffer if needed
    Serial.write((sampleBuffer[index] >> 8) & 0xFF);  // High byte
    Serial.write(sampleBuffer[index] & 0xFF);         // Low byte
  }
}

// Handle serial commands
void handleCommand(char cmd) {
  switch (cmd) {
    case '1':
      currentPattern = SINE_WAVE;
      Serial.println("Pattern: Sine Wave at 440Hz");
      break;
    case '2':
      currentPattern = SQUARE_WAVE;
      Serial.println("Pattern: Square Wave at 440Hz");
      break;
    case '3':
      currentPattern = SAWTOOTH_WAVE;
      Serial.println("Pattern: Sawtooth Wave at 440Hz");
      break;
    case '4':
      currentPattern = WHITE_NOISE;
      Serial.println("Pattern: White Noise");
      break;
    case '5':
      currentPattern = SWEEP;
      Serial.println("Pattern: Frequency Sweep");
      break;
    case '6':
      currentPattern = MICROPHONE;
      Serial.println("Pattern: Microphone Input");
      break;
    case '+':
      if (currentFrequency < 10000) currentFrequency *= 1.1;
      Serial.print("Frequency: ");
      Serial.println(currentFrequency);
      break;
    case '-':
      if (currentFrequency > 20) currentFrequency /= 1.1;
      Serial.print("Frequency: ");
      Serial.println(currentFrequency);
      break;
    case 's':
      // Toggle streaming mode
      streamingToSerial = !streamingToSerial;
      if (streamingToSerial) {
        Serial.println("Streaming audio data to serial port enabled");
        // Automatically switch to microphone if streaming
        if (currentPattern != MICROPHONE) {
          currentPattern = MICROPHONE;
          if (!micInitialized) {
            setupMicrophone();
            micInitialized = true;
          }
          Serial.println("Switched to microphone input for streaming");
        }
      } else {
        Serial.println("Streaming audio data to serial port disabled");
      }
      break;
    case 'h':
      printHelp();
      break;
  }
}

// Print which pattern is currently playing
void printCurrentPattern() {
  Serial.print("\nPlaying: ");
  switch(currentPattern) {
    case SINE_WAVE:
      Serial.println("Sine Wave (440Hz)");
      break;
    case SQUARE_WAVE:
      Serial.println("Square Wave (440Hz)");
      break;
    case SAWTOOTH_WAVE:
      Serial.println("Sawtooth Wave (440Hz)");
      break;
    case WHITE_NOISE:
      Serial.println("White Noise");
      break;
    case SWEEP:
      Serial.println("Frequency Sweep (100Hz-3000Hz)");
      break;
    case MICROPHONE:
      Serial.println("Microphone Input");
      break;
  }
}

// Print help menu
void printHelp() {
  Serial.println("\n--- Commands ---");
  Serial.println("1: Sine wave (440Hz)");
  Serial.println("2: Square wave (440Hz)");
  Serial.println("3: Sawtooth wave (440Hz)");
  Serial.println("4: White noise");
  Serial.println("5: Frequency sweep");
  Serial.println("6: Microphone input");
  Serial.println("s: Toggle streaming audio to serial (for Python visualization)");
  Serial.println("+: Increase frequency");
  Serial.println("-: Decrease frequency");
  Serial.println("h: Show this help");
  Serial.println("--------------");
}
