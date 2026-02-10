# 🔧 Firmware

This directory contains the ESP32-S3 firmware and ground control station files.

## 📁 Contents

| File/Folder | Description |
| :--- | :--- |
| `esp32s3/` | PlatformIO project (main firmware) |
| `index.html` | Web Serial Dashboard (NAV Mission Control) |
| `chart.js` | Chart.js library (bundled locally for offline use) |

## 🚀 Flashing the Firmware

### Prerequisites
- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- USB-C cable

### Steps
1. Open `firmware/esp32s3/` folder in VSCode with PlatformIO.
2. Connect ESP32-S3 SuperMini via USB-C.
3. Click **Upload** (→) in PlatformIO toolbar.

## ⚙️ Configuration

### Serial Baud Rate
The firmware outputs CSV telemetry at **115200 baud** over USB Serial.
Change in `esp32s3.ino`:
```cpp
Serial.begin(115200);
```

### Calibration
- **Gyro Zero**: Automatic on boot (10-second startup, keep module still).
- **Hard Iron Offsets**: Adjust magnetometer offsets in the code for your location.

### CAN Bus
- Speed: **500 kbps**
- TX Pin: `GPIO 5`, RX Pin: `GPIO 6`

## 🖥️ Using the Dashboard
See the [Web Dashboard Guide](../docs/WEB_DASHBOARD.md) for instructions.
