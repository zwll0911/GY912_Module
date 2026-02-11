# 📡 CAN Bus Protocol Specification

[🔙 **Back to Main README**](../README.md)

This document defines the communication protocol for the **Industrial AHRS Navigation Module (V5.1)**.
The module broadcasts orientation telemetry at **50Hz** via the ESP32-S3 TWAI driver.

*   **Baud Rate**: `1 Mbps`
*   **Byte Order**: Big Endian (MSB first)
*   **Format**: Standard ID (11-bit)
*   **Target**: Robomaster C620/C610 speed controllers

---

## 🆔 Message Definition

The module transmits a single CAN frame:

| CAN ID | Description | DLC | Payload Structure (6 Bytes) | Scaling Factor |
| :--- | :--- | :--- | :--- | :--- |
| **0x101** | **Orientation (Euler)** | 6 | `[0-1] Yaw` `[2-3] Pitch` `[4-5] Roll` | `/ 100.0` (Degrees) |

> [!NOTE]
> Only 6 of 8 available bytes are used. Bytes 6-7 are not transmitted (`data_length_code = 6`).

---

## 🧮 Data Interpretation

### Signed Integers
All orientation values are transmitted as **signed 16-bit integers** (`int16_t`).
*   **Range**: -32,768 to +32,767
*   **Example**: A raw value of `14530` for Yaw corresponds to **145.30°**.

### Byte Packing (Big Endian)
Each `int16_t` is packed MSB-first:
```cpp
// From esp32s3.ino
int16_t y = (int16_t)(robotYaw * 100);
message.data[0] = (y >> 8) & 0xFF;  // High byte
message.data[1] = y & 0xFF;         // Low byte
```

### Decoding Logic
To convert CAN bytes back to degrees:
1.  Combine bytes: `int16_t raw = (data[0] << 8) | data[1];`
2.  Convert: `float yaw_deg = raw / 100.0;`

---

## 💻 Decoded Data Example (JSON)

```json
{
  "0x101": {
    "type": "Orientation",
    "yaw_deg": 145.30,
    "pitch_deg": 2.45,
    "roll_deg": -1.10
  }
}
```

---

## ⏱️ Timing

| Parameter | Value |
| :--- | :--- |
| **Transmit Rate** | 50Hz (20ms interval) |
| **Transmit Timeout** | 10ms (`pdMS_TO_TICKS(10)`) |
| **Task Priority** | 2 (below sensor, above LED) |
