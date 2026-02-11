# 📋 Changelog

All notable changes to the **Industrial AHRS Navigation Module** will be documented in this file.

---

## [5.3] — 2026-02-12

### 🔧 Firmware Improvements
- **Yaw Drift Indicator**: Exponential moving average of yaw rate-of-change (°/s) with wraparound handling. Added as CSV field [15].
- **Bidirectional CAN**: Added `twai_receive()` handler for incoming commands on `0x200`:
  - `0x01` → Remote TARE (zero yaw offset).
  - `0x02` + 4-byte float → Set sea-level pressure (saved to NVS).
  - `0x03` → Heartbeat reply on `0x102` with heap KB.
- **OTA Firmware Updates**: WiFi AP mode (`NAV_MODULE_OTA`, password: `navmodule`) with ArduinoOTA. Toggle via serial commands `OTA_ON` / `OTA_OFF`. State persisted in NVS.
- **Configurable CAN**: TX message ID and transmit interval stored in NVS (defaults: `0x101`, 20ms/50Hz).
- **Altitude Smoothing**: 10-sample circular buffer moving average on BMP388 altitude readings.

### 🖥️ Dashboard Improvements
- **SVG Compass Rose**: Real-time rotating compass needle synchronized with yaw heading.
- **Theme Toggle**: Dark (cyberpunk) / Light (high-contrast) theme switch. Saved to `localStorage`. Chart grid colors update with theme.
- **Error Counters**: Status bar showing total lines received, rejected lines, and connection uptime timer.
- **Yaw Drift Display**: Live drift rate (°/s) with color-coded severity (cyan/yellow/red).
- **CSV Recorder**: Now includes drift rate field in exported CSV.

### 🧪 Tests
- **Added** `tests/test_can_packing.py`: 20+ pytest cases for CAN byte packing/unpacking including edge cases, round-trip, and full payload verification.

### 📁 Project Structure
- **Added** `tests/` directory.

---

## [5.2] — 2026-02-12

### 🔧 Firmware Improvements
- **Mutex Optimization**: Moved `Serial.print()` and `bmp.performReading()` outside the mutex lock, reducing CAN task blocking time.
- **CAN Bus Recovery**: Added `twai_get_status_info()` health check and automatic `twai_initiate_recovery()` on bus-off errors.
- **Task Watchdog**: Registered `taskSensor` with ESP32 Task Watchdog Timer (TWDT) for 5-second hang detection.
- **Live Health Telemetry**: Added free heap size and CAN TX error count as CSV fields [13] and [14] (total 15 fields).
- **NVS Settings**: Sea-level pressure reference stored in Non-Volatile Storage via `Preferences` library (default 1013.25 hPa).

### 🖥️ Dashboard Improvements
- **Offline Fix**: Switched Chart.js from CDN to local bundled copy — no internet required.
- **Disconnect Handling**: Dashboard now detects USB disconnection and shows `DISCONNECTED` status.
- **Reconnect Button**: CONNECT button re-appears as RECONNECT after disconnection (no page refresh needed).
- **CSV Flight Recorder**: Added REC/STOP button with pulsing animation — exports timestamped CSV file.
- **Live System Health**: Free Heap (KB) and CAN Bus status now update in real-time from firmware telemetry.

### 📁 Project Structure
- **Added**: `platformio.ini` for PlatformIO users (Arduino IDE remains primary).
- **Added**: `CHANGELOG.md` (this file).

---

## [5.1] — 2026-02-10

### 🔧 Firmware
- **DMP Sensor Fusion**: Offloaded 6-axis fusion to ICM-20948 DMP using `GAME_ROTATION_VECTOR` at 225Hz.
- **3-Task FreeRTOS**: Migrated to `taskSensor` (P3, Core 1), `taskCAN` (P2, Core 0), `taskLED` (P1, Core 0).
- **Thread Safety**: Added `xSemaphoreCreateMutex()` for shared Yaw/Pitch/Roll data.
- **CAN Bus (TWAI)**: Activated at 1 Mbps for Robomaster C620/C610.
- **BMP388 Integration**: Altitude, Pressure, Temperature via SPI at 50Hz.
- **Dual LED Status**: Rainbow cycle (Pin 1) + Purple heartbeat (Pin 48).

### 🖥️ Dashboard (NAV PANEL v5.1)
- **Cyberpunk Theme**: Neon Purple & Cyan palette for competition visibility.
- **Responsive Grid**: CSS Grid layout, 100vh, no scrolling.
- **3D Simulation**: Real-time CSS 3D cube mirroring robot orientation.
- **Multi-Graph System**: Three Chart.js graphs (Yaw, Pitch, Roll).
- **Tare Zero Point**: Instant relative heading reset.
- **Environment Panel**: Live Altitude, Temperature, Pressure display.

---

## [3.0] — 2026-02-08

### 🔧 Firmware
- Initial ESP32-S3 firmware with Madgwick sensor fusion at 100Hz.
- CAN Bus output at 500 kbps.
- Single RGB LED with state-based color patterns.

### 🖥️ Dashboard
- Initial web dashboard with accelerometer, gyroscope, magnetometer panels.
- 3D orientation cube.
- Simulation mode for hardware-free testing.
- Flight recorder with CSV export.

---

## [1.0] — 2025-11-01

### Initial Release
- KiCad PCB design (v1).
- Basic sensor reading and serial output.
