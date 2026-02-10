# Industrial AHRS Navigation Module (V3.0)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-Dual%20Core-red)
![Sensor Fusion](https://img.shields.io/badge/Sensor%20Fusion-Madgwick%20100Hz-blue)
![License](https://img.shields.io/badge/License-MIT-green)

The **Industrial AHRS Navigation Module** is a high-precision flight controller core designed for advanced robotics applications. It leverages the ESP32-S3's dual-core architecture to fuse data from a 9-DOF IMU and a precision barometer, delivering stable orientation (Yaw, Pitch, Roll) and altitude telemetry via CAN Bus.

## 🚀 Key Features

*   **Dual-Core Architecture**: Dedicated cores for real-time sensor fusion (Core 1) and communication/UI (Core 0).
*   **High-Speed Fusion**: 100Hz Madgwick filter update rate for responsive tracking.
*   **Auto-Calibration**: Intelligent startup routine to zero gyroscope drift and correct hard-iron magnetic distortion.
*   **Robust Telemetry**: CAN Bus (TWAI) output at 500kbps for reliable data transmission.
*   **Visual Status**: Intuitive RGB LED color codes for system state and health monitoring.

## 📂 Project Structure

*   `firmware/`: Source code for the ESP32-S3.
*   `docs/`: Detailed documentation resources.
    *   [Hardware & Pinout](docs/HARDWARE.md)
    *   [CAN Protocol Spec](docs/CAN_PROTOCOL.md)
    *   [System Architecture](docs/ARCHITECTURE.md)
*   `v1/`: KiCad PCB design files (schematics and layout).

## 🛠️ Hardware Specifications

| Component | Model | Function | Bus |
| :--- | :--- | :--- | :--- |
| **MCU** | ESP32-S3 SuperMini | Main Processor | - |
| **IMU** | ICM-20948 | Accel / Gyro / Mag | SPI |
| **Barometer** | BMP388 | Altitude / Temp | SPI |
| **Transceiver** | SN65HVD230 | CAN Bus Interface | UART |

## 🚦 Status Indicators

| Color | Pattern | System State |
| :--- | :--- | :--- |
| 🟠 Orange | Solid | **Calibrating** (Keep Still) |
| 🟢 Green | Flash | **Ready** (Calibration Done) |
| 🟣 Purple | Breathing | **active** / Idle |
| 🟣 Purple | Fast Flash | **Transmitting** Data |

## 📦 Getting Started

1.  **Clone the Repo**:
    ```bash
    git clone https://github.com/zwll0911/GY912_Module.git
    ```
2.  **Wiring**: Connect sensors as per the [Hardware Guide](docs/HARDWARE.md).
3.  **Build & Flash**: Use PlatformIO to upload the firmware.
4.  **Connect CAN**: Hook up the CAN H/L lines to your robot's bus (500kbps).

## 📄 License

MIT License - see [LICENSE](LICENSE) for details.
