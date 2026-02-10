# 🏗️ System Architecture

[🔙 **Back to Main README**](../README.md)

This document explains the software design choices and data flow of the **Industrial AHRS Navigation Module (V3.0)**.

---

## ⚡ Dual-Core FreeRTOS Design

The ESP32-S3's dual-core architecture is leveraged to separate critical real-time sensor fusion from non-critical communication tasks.

| Core | Task Name | Priority | Responsibilities |
| :--- | :--- | :--- | :--- |
| **Core 1** | `Task_Sensors` | **High** | Sensor Data Acquisition (SPI) <br> Hard Iron Calibration <br> Madgwick Sensor Fusion (100Hz) <br> CAN Bus Broadcast |
| **Core 0** | `Task_LEDs` | **Low** | RGB LED Animations (NeoPixel) <br> WiFi WebSockets <br> Web Dashboard Hosting |

> [!TIP]
> **Why this split?**
> Sensor fusion requires strict timing (10ms loop). LED animations (WS2812B) disable interrupts during data transmission, which would cause jitter in our fusion algorithm. By placing them on separate cores, the navigation loop remains jitter-free.

---

## ⚙️ Calibration Routine

### 1. Startup Sequence
The system performs a self-check on boot:
*   **Power On**: System boots, **Orange LED** illuminates.
*   **Gyro Zeroing (10s)**: The system collects 1000 samples of gyroscope data while stationary to determine the zero-rate offset.

### 2. Hard Iron Calibration
Magnetometer data is corrected for hard-iron distortions using pre-calculated offsets stored in EEPROM/Flash.

### 3. Fusion Start
Once calibrated, the **Green LED** flashes, and the Madgwick filter begins integration.

---

## 🔄 System Data Flow

The following diagram illustrates how data moves through the dual-core system:

```mermaid
flowchart LR
    subgraph Core1 ["Core 1: Real-Time Fusion"]
        direction TB
        Acquisition[Data Acquisition]
        Calib[Calibration Engine]
        Madgwick[Madgwick Filter]
        Euler[Euler Angles]
        CAN_Task[CAN Bus Task]
    end

    subgraph Core0 ["Core 0: UI & Comms"]
        LEDs[LED Animation]
        Dashboard[Web Dashboard]
    end

    subgraph Hardware
        Sensors[IMU / Baro]
        Bus((CAN Bus))
        RGB_LED((RGB LED))
    end

    %% Data Flow
    Sensors -->|SPI Data| Acquisition
    Acquisition -->|Raw Accel/Gyro| Calib
    Calib -->|Corrected Data| Madgwick
    Madgwick -->|Quaternion| Euler
    Euler -->|Yaw/Pitch| CAN_Task
    CAN_Task -->|CAN Frames| Bus
    
    %% Status Indication
    Calib -.->|State Change| LEDs
    Madgwick -.->|Status| LEDs
    LEDs --> RGB_LED
```
