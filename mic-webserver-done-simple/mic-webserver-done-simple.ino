#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <driver/i2s.h>

// ←── Edit these ──→
const char* SSID = "SBN-5B-2.4G";
const char* PASS = "sbn5@1234567";

// I2S mic pins (INMP441)
#define I2S_WS   25   // Word select (LRCK)
#define I2S_SCK  32   // Bit clock (BCLK)
#define I2S_SD   33   // Serial data (DOUT)
#define I2S_NUM  I2S_NUM_0

WebServer  server(80);
WebSocketsServer  ws(81);

// The web page served to clients:
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Mic Waveform</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style> canvas { width:100%; height:300px; } </style>
</head>
<body>
  <h2>Live Mic Waveform</h2>
  <canvas id="waveChart"></canvas>
  <script>
    const ctx = document.getElementById('waveChart').getContext('2d');
    const data = {
      labels: Array(64).fill(''),
      datasets: [{ label:'Amplitude', data:Array(64).fill(0),
        borderColor:'rgb(0,123,255)', borderWidth:1, pointRadius:0 }]
    };
    const chart = new Chart(ctx,{ type:'line', data:data, options:{
      animation:false,
      scales:{ x:{display:false},
        y:{ suggestedMin:-32768, suggestedMax:32767 } } }
    });
    const sock = new WebSocket(`ws://${location.host}:81/`);
    sock.onmessage = evt => {
      const samples = JSON.parse(evt.data);
      chart.data.datasets[0].data = samples;
      chart.update();
    };
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300); Serial.print('.');
  }
  Serial.println("\nConnected! IP = " + WiFi.localIP().toString());

  // I²S microphone config (mono, 16-bit, 16 kHz)
  i2s_config_t i2s_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 128,
    .use_apll = false
  };
  i2s_driver_install(I2S_NUM, &i2s_cfg, 0, nullptr);

  // I²S pin mapping
  i2s_pin_config_t pin_cfg = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };
  i2s_set_pin(I2S_NUM, &pin_cfg);

  // Start HTTP + WebSocket
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  ws.begin();
  ws.onEvent([](uint8_t num, WStype_t type, uint8_t *pkt, size_t len){
    /* no incoming messages needed */
  });
}

void loop() {
  server.handleClient();
  ws.loop();

  // Read 64 samples from mic
  static int16_t buf[64];
  size_t bytesRead;
  i2s_read(I2S_NUM, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
  int count = bytesRead / sizeof(int16_t);

  // Build JSON array string
  String msg = "[";
  for (int i = 0; i < count; i++) {
    msg += buf[i];
    if (i < count - 1) msg += ',';
  }
  msg += "]";

  // Broadcast to all WebSocket clients
  ws.broadcastTXT(msg);
}
