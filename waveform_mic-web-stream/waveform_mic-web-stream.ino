#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <cmath>  // For sqrt()

// ←── WiFi Settings ──→
const char* SSID = "your";
const char* PASS = "password";

// I2S mic pins (INMP441)
#define I2S_WS   25
#define I2S_SCK  32
#define I2S_SD   33
#define I2S_NUM  I2S_NUM_0

// Audio Settings - keep consistent across application
#define SAMPLE_RATE 32000      // Improved for web audio, better than 16kHz for speech/music
#define BUFFER_SIZE 512        // Larger buffer for smoother, higher quality playback
#define JSON_BUFFER_SIZE 4096  // Increase for larger buffer

// Environment classification thresholds
#define CALM_THRESHOLD 500
#define NOISY_THRESHOLD 5000

WebServer server(80);
WebSocketsServer ws(81);
bool clientConnected = false;
unsigned long lastDataSent = 0;
unsigned long dataInterval = 30;  // Send data every 30ms for smoother playback

// Audio analysis variables
float currentRMS = 0;
float currentPeak = 0;
float highFreqEnergy = 0;
float lowFreqEnergy = 0;
String environment = "Normal";  // Default environment

// Function prototypes
float calculateRMS(int16_t* samples, size_t count);
float calculatePeak(int16_t* samples, size_t count);
void calculateFrequencyDistribution(int16_t* samples, size_t count);
String classifyEnvironment(float rms, float lowEnergy, float highEnergy);

// Simple IIR low-pass filter (alpha: 0.2-0.5 for speech, adjust as needed)
void iirLowPassFilter(int16_t* in, int16_t* out, size_t count, float alpha) {
  out[0] = in[0];
  for (size_t i = 1; i < count; i++) {
    out[i] = (int16_t)(alpha * in[i] + (1.0f - alpha) * out[i-1]);
  }
}

float calculateRMS(int16_t* samples, size_t count) {
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += samples[i] * samples[i];
  }
  return sqrt(sum / count);
}

float calculatePeak(int16_t* samples, size_t count) {
  float peak = 0;
  for (int i = 0; i < count; i++) {
    float abs_sample = abs(samples[i]);
    if (abs_sample > peak) peak = abs_sample;
  }
  return peak;
}

void calculateFrequencyDistribution(int16_t* samples, size_t count) {
  // Simple frequency analysis (low vs high)
  lowFreqEnergy = 0;
  highFreqEnergy = 0;
  
  for (int i = 1; i < count; i++) {
    // Calculate derivative - higher derivatives suggest higher frequencies
    float derivative = abs(samples[i] - samples[i-1]);
    
    if (derivative < 1000) {
      lowFreqEnergy += derivative;
    } else {
      highFreqEnergy += derivative;
    }
  }
  
  // Normalize
  lowFreqEnergy /= count;
  highFreqEnergy /= count;
}

String classifyEnvironment(float rms, float lowEnergy, float highEnergy) {
  if (rms < CALM_THRESHOLD || (lowEnergy > highEnergy * 2)) {
    return "Calm";
  } else if (rms > NOISY_THRESHOLD || (highEnergy > lowEnergy * 2)) {
    return "Noisy";
  } else {
    return "Normal";
  }
}

// The web page with enhanced visualization:
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Audio Analyzer</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 1000px; margin: 0 auto; }
    .charts { display: flex; flex-direction: column; gap: 20px; }
    .chart-container { background: #f8f9fa; border-radius: 8px; padding: 15px; }
    canvas { width:100%; height:250px; }
    .metrics { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 10px; margin-bottom: 20px; }
    .metric { background: #f8f9fa; border-radius: 8px; padding: 15px; text-align: center; }
    .metric-value { font-size: 24px; font-weight: bold; margin: 10px 0; }
    .environment { font-size: 32px; text-align: center; margin: 20px 0; }
    .environment.calm { color: #2ecc71; }
    .environment.normal { color: #3498db; }
    .environment.noisy { color: #e74c3c; }
    #connectionStatus { position: fixed; top: 10px; right: 10px; padding: 5px 10px; border-radius: 4px; }
    .connected { background: #2ecc71; color: white; }
    .disconnected { background: #e74c3c; color: white; }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Audio Analyzer</h1>
    <div id="connectionStatus" class="disconnected">Disconnected</div>
    <div id="environment-display" class="environment normal">Normal</div>
    <div class="metrics">
      <div class="metric">
        <h3>RMS Level</h3>
        <div id="rms-value" class="metric-value">0</div>
      </div>
      <div class="metric">
        <h3>Peak Level</h3>
        <div id="peak-value" class="metric-value">0</div>
      </div>
      <div class="metric">
        <h3>Low/High Ratio</h3>
        <div id="lh-ratio" class="metric-value">0</div>
      </div>
    </div>
    <div class="charts">
      <div class="chart-container">
        <h2>Waveform</h2>
        <canvas id="waveChart"></canvas>
      </div>
      <div class="chart-container">
        <h2>Audio Level History</h2>
        <canvas id="historyChart"></canvas>
      </div>
    </div>
  </div>
  <script>
    // JavaScript code for WebSocket and charts
    const waveCtx = document.getElementById('waveChart').getContext('2d');
    const waveData = {
      labels: Array(128).fill(''), // match BUFFER_SIZE
      datasets: [{
        label: 'Amplitude',
        data: Array(128).fill(0), // match BUFFER_SIZE
        borderColor: 'rgb(0,123,255)',
        borderWidth: 2,
        pointRadius: 0
      }]
    };
    const waveChart = new Chart(waveCtx, {
      type: 'line',
      data: waveData,
      options: {
        responsive: true,
        animation: false,
        scales: {
          x: { display: false },
          y: { suggestedMin: -32768, suggestedMax: 32767 }
        }
      }
    });

    const historyCtx = document.getElementById('historyChart').getContext('2d');
    const historyData = {
      labels: Array(100).fill(''),
      datasets: [{
        label: 'RMS Level',
        data: Array(100).fill(0),
        borderColor: 'rgb(76,175,80)',
        backgroundColor: 'rgba(76,175,80,0.2)',
        borderWidth: 1,
        pointRadius: 0,
        fill: true
      }]
    };
    const historyChart = new Chart(historyCtx, {
      type: 'line',
      data: historyData,
      options: {
        responsive: true,
        animation: false,
        scales: {
          x: { display: false },
          y: { suggestedMin: 0, suggestedMax: 5000 }
        }
      }
    });

    let socket = new WebSocket('ws://' + window.location.hostname + ':81');
    socket.onopen = () => {
      document.getElementById('connectionStatus').classList.remove('disconnected');
      document.getElementById('connectionStatus').classList.add('connected');
    };
    socket.onclose = () => {
      document.getElementById('connectionStatus').classList.remove('connected');
      document.getElementById('connectionStatus').classList.add('disconnected');
    };
    // Add audio streaming
    let audioCtx, audioBufferQueue = [];
    let streaming = false;
    let streamBtn = document.createElement('button');
    streamBtn.innerText = 'Start Audio Stream';
    streamBtn.style.margin = '10px';
    document.body.insertBefore(streamBtn, document.body.firstChild);

    streamBtn.onclick = function() {
      streaming = !streaming;
      streamBtn.innerText = streaming ? 'Stop Audio Stream' : 'Start Audio Stream';
      if (streaming && !audioCtx) {
        audioCtx = new (window.AudioContext || window.webkitAudioContext)({sampleRate: 32000});
      }
    };

    socket.binaryType = 'arraybuffer';
    socket.onmessage = function (event) {
      if (typeof event.data === 'string') {
        const data = JSON.parse(event.data);
        document.getElementById('rms-value').innerText = data.rms;
        document.getElementById('peak-value').innerText = data.peak;
        document.getElementById('lh-ratio').innerText = (data.lowEnergy / data.highEnergy).toFixed(2);

        document.getElementById('environment-display').classList = 'environment ' + data.environment.toLowerCase();
        document.getElementById('environment-display').innerText = data.environment;

        // Update charts
        waveData.datasets[0].data = data.waveform;
        waveChart.update();

        historyData.datasets[0].data.push(data.rms);
        if (historyData.datasets[0].data.length > 100) {
          historyData.datasets[0].data.shift();
        }
        historyChart.update();
      } else if (streaming && audioCtx) {
        // Play raw PCM audio (16-bit signed, mono, 32kHz)
        let pcm = new Int16Array(event.data);
        let float32 = new Float32Array(pcm.length);
        for (let i = 0; i < pcm.length; i++) {
          float32[i] = pcm[i] / 32768;
        }
        let buffer = audioCtx.createBuffer(1, float32.length, 32000);
        buffer.copyToChannel(float32, 0);
        let src = audioCtx.createBufferSource();
        src.buffer = buffer;
        src.connect(audioCtx.destination);
        src.start();
      }
    };
  </script>
</body>
</html>
)rawliteral";

// WebSocket event handler
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      clientConnected = false;
      Serial.println("WebSocket client disconnected.");
      break;
    case WStype_CONNECTED:
      clientConnected = true;
      Serial.print("WebSocket client connected: ");
      Serial.println(num);
      break;
    case WStype_ERROR:
      clientConnected = false;
      Serial.println("WebSocket error.");
      break;
    default:
      break;
  }
}

// Use this macro to bypass filtering if you want pure raw audio
#define ENABLE_FILTER 1

// Setup Wi-Fi, WebSocket, and I2S
void setup() {
  Serial.begin(115200);

  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());

  ws.begin();
  ws.onEvent(onWsEvent);

  // Set up I2S (improved for quality)
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
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_NUM, &pin_config);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });
  server.begin();
}

void loop() {
  ws.loop();
  server.handleClient();

  int16_t buffer[BUFFER_SIZE] = {0};
  int16_t filteredBuffer[BUFFER_SIZE] = {0};
  size_t bytesRead = 0;

  // Read audio samples from I2S
  esp_err_t result = i2s_read(I2S_NUM, (void*)buffer, BUFFER_SIZE * sizeof(int16_t), &bytesRead, portMAX_DELAY);
  if (result != ESP_OK) {
    Serial.print("I2S read error: ");
    Serial.println(result);
    delay(dataInterval);
    return;
  }
  if (bytesRead != BUFFER_SIZE * sizeof(int16_t)) {
    Serial.print("Partial I2S read: ");
    Serial.println(bytesRead);
    memset(buffer + (bytesRead / 2), 0, BUFFER_SIZE - (bytesRead / 2));
  }
  Serial.print("Bytes read: ");
  Serial.println(bytesRead);

#if ENABLE_FILTER
  iirLowPassFilter(buffer, filteredBuffer, BUFFER_SIZE, 0.25f); // Lower alpha for less distortion
#else
  memcpy(filteredBuffer, buffer, BUFFER_SIZE * sizeof(int16_t));
#endif

  // Audio analysis
  currentRMS = calculateRMS(filteredBuffer, BUFFER_SIZE);
  currentPeak = calculatePeak(filteredBuffer, BUFFER_SIZE);
  calculateFrequencyDistribution(filteredBuffer, BUFFER_SIZE);
  environment = classifyEnvironment(currentRMS, lowFreqEnergy, highFreqEnergy);

  // Downsample waveform for web (max 256 points for higher detail)
  const int waveformLen = 256;
  int16_t waveform[waveformLen];
  for (int i = 0; i < waveformLen; i++) {
    int idx = (i * BUFFER_SIZE) / waveformLen;
    waveform[i] = filteredBuffer[idx];
  }

  // Create JSON response
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonResponse;
  jsonResponse["rms"] = currentRMS;
  jsonResponse["peak"] = currentPeak;
  jsonResponse["lowEnergy"] = lowFreqEnergy;
  jsonResponse["highEnergy"] = highFreqEnergy;
  jsonResponse["environment"] = environment;
  JsonArray wf = jsonResponse.createNestedArray("waveform");
  for (int i = 0; i < waveformLen; i++) {
    wf.add(waveform[i]);
  }

  // Send to WebSocket
  if (clientConnected) {
    String jsonStr;
    serializeJson(jsonResponse, jsonStr);
    ws.sendTXT(0, jsonStr);
    ws.sendBIN(0, (uint8_t*)filteredBuffer, BUFFER_SIZE * sizeof(int16_t));
    Serial.println("Data sent to client.");
  } else {
    Serial.println("No WebSocket client connected.");
  }

  delay(dataInterval);
}
