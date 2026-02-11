# 🔧 Firmware

This directory contains the ESP32-S3 firmware and ground control station files for the **Industrial AHRS Navigation Module (V5.1)**.

## 📁 Contents

| File/Folder | Description | Size |
| :--- | :--- | :--- |
| `esp32s3/esp32s3.ino` | Main firmware (FreeRTOS, DMP, CAN, LED) | ~6.7 KB |
| `index.html` | NAV PANEL v5.1 (Web Serial Dashboard) | ~16.4 KB |
| `chart.js` | Chart.js library (bundled for offline use) | ~208 KB |

## 🚀 Flashing the Firmware

### Prerequisites
- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- USB-C cable

### Required Libraries
| Library | Purpose |
| :--- | :--- |
| `SparkFun ICM-20948` | DMP sensor fusion driver |
| `Adafruit BMP3XX` | BMP388 barometer driver |
| `Adafruit NeoPixel` | WS2812B RGB LED control |
| `Adafruit Unified Sensor` | Sensor abstraction layer |

### Steps
1. Open `firmware/esp32s3/` folder in VSCode with PlatformIO.
2. Connect ESP32-S3 SuperMini via USB-C.
3. Click **Upload** (→) in PlatformIO toolbar.

## ⚙️ Configuration

### Serial Baud Rate
The firmware outputs CSV telemetry at **115200 baud** over USB Serial:
```cpp
Serial.begin(115200);
```

### DMP Initialization
The ICM-20948 DMP is configured for maximum responsiveness:
```cpp
myICM.begin(PIN_CS_ACCEL, SPI, 3000000);                               // SPI @ 3 MHz
myICM.initializeDMP();                                                   // Load DMP firmware
myICM.enableDMPSensor(INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR, true);  // 6-axis fusion
myICM.setDMPODRrate(DMP_ODR_Reg_Quat6, 0);                             // 225 Hz (max)
myICM.enableFIFO();                                                      // FIFO routing
myICM.enableDMP();                                                       // Start DMP
myICM.resetDMP();                                                        // Clear state
myICM.resetFIFO();                                                       // Flush buffer
```

### BMP388 Barometer
```cpp
bmp.begin_SPI(PIN_CS_BARO, &SPI);
bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);   // High accuracy temp
bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);      // Balanced pressure
bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);        // Noise filtering
bmp.setOutputDataRate(BMP3_ODR_50_HZ);                  // 50 Hz output
```

**Altitude reference**: ISA sea-level pressure of **1013.25 hPa**. Adjust for local conditions if needed:
```cpp
alt = bmp.readAltitude(1013.25);  // Change this value for your location
```

### Calibration
- **DMP Init**: Onboard LED turns **solid red** if DMP initialization fails (system halts).
- **Gyro Zeroing**: Keep the module **still during boot** — DMP startup calibration eliminates drift.
- **No magnetometer**: GAME_ROTATION_VECTOR uses only Accel + Gyro (no magnetic interference).

### CAN Bus (TWAI)
| Parameter | Value |
| :--- | :--- |
| Speed | **1 Mbps** (`TWAI_TIMING_CONFIG_1MBITS()`) |
| TX Pin | `GPIO 5` |
| RX Pin | `GPIO 6` |
| Mode | Normal |
| Filter | Accept All |
| Message ID | `0x101` (Euler angles, 6 bytes, big-endian) |

### Pin Map

| Pin | Function | Direction |
| :--- | :--- | :--- |
| 12 | SPI SCK | Output |
| 13 | SPI MISO | Input |
| 11 | SPI MOSI | Output |
| 10 | CS_IMU (ICM-20948) | Output |
| 9 | CS_BARO (BMP388) | Output |
| 5 | CAN_TX | Output |
| 6 | CAN_RX | Input |
| 1 | External RGB LED | Output |
| 48 | Onboard RGB LED | Output |

### Serial Output Format (CSV, 13 fields)
```
0.0, 0.0, 0.0, Pitch, Roll, Yaw, Yaw, 0.0, 0.0, 0.0, Temp(°C), Pressure(hPa), Altitude(m)
```

## 🖥️ Using the Dashboard
See the [Web Dashboard Guide](../docs/WEB_DASHBOARD.md) for complete instructions.
