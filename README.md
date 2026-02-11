# 🚁 Industrial AHRS Navigation Module (V5.1)

![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3-red?style=for-the-badge&logo=espressif)
![Sensor Fusion](https://img.shields.io/badge/Algorithm-DMP_225Hz-blue?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

> **High-Precision Navigation Core for Competition Robotics (Robocon)**

The **Industrial AHRS Navigation Module** is a robust sensor fusion engine built on the **ESP32-S3** dual-core MCU. It offloads 6-axis sensor fusion to the **ICM-20948 DMP** (GAME_ROTATION_VECTOR) and integrates a **BMP388** precision barometer to deliver stable, drift-free orientation (Yaw, Pitch, Roll) and environment telemetry via **1 Mbps CAN Bus** (TWAI).

---

## 📑 Table of Contents

- [Key Features](#-key-features)
- [System Status Indicators](#-system-status-indicators)
- [Hardware Specs](#️-hardware-specifications)
- [Quick Start](#-quick-start)
- [Documentation](#-documentation)
- [Project Structure](#-project-structure)

---

## 🚀 Key Features

*   **⚡ DMP Sensor Fusion**: Offloaded to the ICM-20948 Digital Motion Processor at **225Hz** (0 ODR divisor) for maximum responsiveness.
*   **🧠 3-Task FreeRTOS Architecture**: `taskSensor` (Core 1, 200Hz), `taskCAN` (Core 0, 50Hz), `taskLED` (Core 0) — each with dedicated priorities and thread-safe mutex sharing.
*   **🎯 Yaw Stabilization**: Startup calibration logic eliminates gyro drift for a stable 0.0° heading reference.
*   **📡 High-Speed CAN Bus**: TWAI driver at **1 Mbps** for communication with Robomaster C620/C610 speed controllers.
*   **🌡️ Environment Monitoring**: BMP388 precision barometer for live Altitude, Pressure, and Temperature.
*   **🌈 Dual LED Status**: Rainbow cycle (Pin 1) for system idle + Neon Purple heartbeat (Pin 48) for CPU alive.

## 💡 System Status Indicators

The module uses dual RGB LEDs to communicate its current state.

| LED | Pin | Pattern | System State |
| :--- | :--- | :--- | :--- |
| 🌈 **External** | **1** | **Rainbow Cycle** (HSV loop) | System Idle / Ready |
| 🟣 **Onboard** | **48** | **Purple Blink** (0.5Hz) | CPU Alive / Heartbeat |
| 🔴 **Onboard** | **48** | **Solid Red** | DMP Init Failure |

## 🛠️ Hardware Specifications

| Component | Model | Function | Bus Interface |
| :--- | :--- | :--- | :--- |
| **MCU** | [ESP32-S3 SuperMini](https://www.espressif.com/en/products/socs/esp32-s3) | Main Processor | - |
| **IMU** | **ICM-20948** | 6-DOF Accel/Gyro (DMP Fusion) | SPI (High Speed) |
| **Barometer** | **BMP388** | Precision Altitude | SPI |
| **Transceiver** | **SN65HVD230** | CAN Bus Interface | TWAI |

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
Hook up the `CAN H` and `CAN L` lines to your robot's bus network (**1 Mbps**).

## 📚 Documentation

Detailed documentation is available in the `docs/` directory:

*   📖 **[Hardware & Pinout Guide](docs/HARDWARE.md)** - Wiring diagrams and pin maps.
*   📡 **[CAN Protocol Specification](docs/CAN_PROTOCOL.md)** - Message IDs and data formats.
*   🏗️ **[System Architecture](docs/ARCHITECTURE.md)** - FreeRTOS task design and data flow.
*   🖥️ **[Web Dashboard Guide](docs/WEB_DASHBOARD.md)** - Real-time telemetry UI.
*   🔩 **[PCB Design](docs/PCB_DESIGN.md)** - Circuit block diagram and component BOM.

## 📂 Project Structure

```text
.
├── firmware/       # ESP32-S3 Source Code & Web Dashboard
├── docs/           # Documentation Resources
├── v1/             # KiCad PCB Design Files
└── README.md       # This file
```

---

<p align="center">
  Made with ❤️ for Robocon
</p>
