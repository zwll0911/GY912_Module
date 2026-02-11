# 🔧 Firmware

This directory contains the ESP32-S3 firmware and ground control station files.

## 📁 Contents

| File/Folder | Description |
| :--- | :--- |
| `esp32s3/` | Arduino project (main firmware) |
| `index.html` | Web Serial Dashboard (NAV PANEL v5.1) |
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
- **DMP Initialization**: The ICM-20948 DMP is initialized on boot with `GAME_ROTATION_VECTOR` enabled.
- **Gyro Zeroing**: Startup calibration logic eliminates drift — keep the module still during boot.
- **Failure Indicator**: Onboard LED turns solid **red** if DMP initialization fails.

### CAN Bus
- Speed: **1 Mbps**
- TX Pin: `GPIO 5`, RX Pin: `GPIO 6`
- Driver: ESP32-S3 TWAI (built-in)

## 🖥️ Using the Dashboard
See the [Web Dashboard Guide](../docs/WEB_DASHBOARD.md) for instructions.
