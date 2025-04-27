# ESP32 Mic Audio Analyzer

A real-time audio analyzer and streamer using ESP32 and Python.  
Stream high-quality audio from an I2S microphone (e.g., INMP441) on ESP32 to your PC via USB serial, visualize and analyze the audio, and enjoy robust noise reduction and environment classification.

---

## Features

- **Real-time audio streaming** from ESP32 to PC via USB serial (16kHz, 16-bit, mono)
- **Live waveform and RMS plotting** using Python and matplotlib
- **Environment classification** (Calm, Noisy, Speech, etc.)
- **Active noise reduction** (DC removal, lowpass, and spectral gating)
- **Low-latency, high-fidelity playback** on your PC
- **No web server or browser required**

---

## Hardware Requirements

- ESP32 board
- I2S microphone (e.g., INMP441)
- USB cable for ESP32-PC connection

---

## Software Requirements

- Python 3.8+
- [sounddevice](https://python-sounddevice.readthedocs.io/)
- [numpy](https://numpy.org/)
- [matplotlib](https://matplotlib.org/)
- [scipy](https://scipy.org/)
- [pyserial](https://pyserial.readthedocs.io/)

Install dependencies:
```bash
pip install sounddevice numpy matplotlib scipy pyserial
```

---

## Getting Started

### 1. Flash the ESP32

Upload `ESP32_Mic_Analyzer.ino` to your ESP32 using Arduino IDE or PlatformIO.

### 2. Connect the Microphone

Wire your INMP441 (or compatible) I2S mic to the ESP32 as per the pin definitions in the `.ino` file.

### 3. Run the Python Analyzer

Edit `SERIAL_PORT` in `serial_audio_analyze.py` to match your ESP32's COM port (e.g., `COM5` or `/dev/ttyUSB0`).

Run:
```bash
python serial_audio_analyze.py
```

You will see live plots and hear the processed audio on your PC.

---

## How It Works

- The ESP32 reads audio from the I2S mic and streams raw PCM data over serial.
- The Python script reads the audio, applies noise reduction and filtering, visualizes the waveform and RMS, and plays the audio in real time.
- The analyzer classifies the environment and displays it live.

---

## Advanced

- **Noise reduction**: Uses DC blocking, lowpass filtering, and spectral gating for clean playback.
- **Environment classification**: Uses both amplitude and spectral features.
- **Low latency**: Small buffers and efficient processing for near real-time feedback.

---

## Troubleshooting

- If you hear noise or glitches, try lowering the baud rate or buffer size.
- If you get serial errors, make sure no other program is using the COM port.
- For best results, use a shielded USB cable and keep the mic away from noise sources.

---

## License

MIT License

---

## Credits

- [noisereduce](https://github.com/timsainb/noisereduce) (algorithm inspiration)
- [sounddevice](https://python-sounddevice.readthedocs.io/)
- [matplotlib](https://matplotlib.org/)
- [ESP32 Arduino core](https://github.com/espressif/arduino-esp32)

---

## Contributing

Pull requests and suggestions are welcome!  
If you build something cool with this, share it on GitHub and let the community know!

---
