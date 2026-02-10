# 🚁 Industrial AHRS Navigation Module (V3.0)

![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3-red?style=for-the-badge&logo=espressif)
![Sensor Fusion](https://img.shields.io/badge/Algorithm-Madgwick_100Hz-blue?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

> **High-Precision Flight Controller Core for Advanced Robotics**

The **Industrial AHRS Navigation Module** is a robust sensor fusion engine built on the **ESP32-S3** dual-core MCU. It fuses data from a 9-DOF IMU and a precision barometer to deliver stable, drift-free orientation (Yaw, Pitch, Roll) and altitude telemetry via high-speed CAN Bus.

---

## � Table of Contents

- [Key Features](#-key-features)
- [System Status](#-system-status-indicators)
- [Hardware Specs](#-hardware-specifications)
- [Quick Start](#-quick-start)
- [Documentation](#-documentation)
- [Project Structure](#-project-structure)

---

## 🚀 Key Features

*   **⚡ Dual-Core Power**: Dedicated cores for real-time sensor fusion (Core 1) and communication/UI (Core 0).
*   **🎯 Precision Fusion**: 100Hz Madgwick filter update rate ensures responsive and accurate tracking.
*   **🧠 Auto-Calibration**: Intelligent startup routine zeroes gyroscope drift and corrects hard-iron magnetic distortion.
*   **bw Robust Telemetry**: Industrial CAN Bus (TWAI) output at **500kbps** for reliable data transmission.
*   **🌈 Visual Feedback**: Intuitive RGB LED color codes for instant system status monitoring.

## � System Status Indicators

The module uses RGB LEDs to communicate its current state.

| Color | Pattern | System State | Action Required |
| :--- | :--- | :--- | :--- |
| 🟠 **Orange** | **Solid** | **Calibrating** | **Keep module still** (~10s) |
| 🟢 **Green** | **Flash** | **Ready** | Calibration complete, ready to fly |
| 🟣 **Purple** | **Breathing** | **Active / Idle** | Normal operation |
| 🟣 **Purple** | **Fast Flash** | **Transmitting** | Sending telemetry data |

## 🛠️ Hardware Specifications

| Component | Model | Function | Bus Interface |
| :--- | :--- | :--- | :--- |
| **MCU** | [ESP32-S3 SuperMini](https://www.espressif.com/en/products/socs/esp32-s3) | Main Processor | - |
| **IMU** | **ICM-20948** | 9-DOF Accel/Gyro/Mag | SPI (High Speed) |
| **Barometer** | **BMP388** | Precision Altitude | SPI |
| **Transceiver** | **SN65HVD230** | CAN Bus Interface | UART |

## ⚡ Quick Start

### 1. Clone & Setup
```bash
git clone https://github.com/zwll0911/GY912_Module.git
cd GY912_Module
```

### 2. Wiring
Connect your sensors as per the [Hardware Guide](docs/HARDWARE.md). **Ensure 3.3V logic compatibility!**

### 3. Build & Flash
Use **PlatformIO** to compile and upload the firmware to your ESP32-S3.

### 4. Connect CAN
Hook up the `CAN H` and `CAN L` lines to your robot's bus network (500kbps).

## � Documentation

Detailed documentation is available in the `docs/` directory:

*   📖 **[Hardware & Pinout Guide](docs/HARDWARE.md)** - Wiring diagrams and pin maps.
*   📡 **[CAN Protocol Specification](docs/CAN_PROTOCOL.md)** - Message IDs and data formats.
*   🏗️ **[System Architecture](docs/ARCHITECTURE.md)** - Internal design and data flow.

## 📂 Project Structure

```text
.
├── firmware/       # ESP32-S3 Source Code
├── docs/           # Documentation Resources
├── v1/             # KiCad PCB Design Files
└── README.md       # This file
```

---

<p align="center">
  Made with ❤️ for Robocon
</p>
