# 📡 CAN Bus Protocol Specification

[🔙 **Back to Main README**](../README.md)

This document defines the communication protocol for the **Industrial AHRS Navigation Module (V3.0)**.
The module broadcasts telemetry data at a fixed rate.

*   **Baud Rate**: `500 kbps`
*   **Endianness**: Little Endian (Standard for x86/ARM/ESP32)
*   **Format**: Standard ID (11-bit)

---

## 🆔 Message IDs & Payload Definition

| CAN ID | Description | Data Type | Payload Structure (8 Bytes) | Scaling Factor |
| :--- | :--- | :--- | :--- | :--- |
| **0x100** | **Raw Accelerometer** | `int16_t` | `[0-1] Ax` `[2-3] Ay` `[4-5] Az` `[6-7] Res` | `/ 1000.0` (G-force) |
| **0x101** | **Orientation (Euler)** | `int16_t` | `[0-1] Pitch` `[2-3] Roll` `[4-5] Yaw` `[6-7] Res` | `/ 100.0` (Degrees) |
| **0x102** | **Altitude & Baro** | `int16_t` | `[0-1] Alt` `[2-3] Temp` `[4-7] Reserved` | `/ 100.0` (Meters / °C) |

> [!NOTE]
> **Reserved Bytes**: Bytes marked as `Res` or `Reserved` are currently unused and set to `0x00`.

---

## 🧮 Data Interpretation

### Signed Integers
All sensor values are transmitted as **signed 16-bit integers** (`int16_t`).
*   **Range**: -32,768 to +32,767
*   **Example**: A raw value of `-1500` for Pitch corresponds to `-15.00` degrees.

### Decoding Logic
To convert the raw CAN bytes back to real-world units:

1.  Combine the **Low Byte** and **High Byte** to form a 16-bit integer.
2.  Cast to a signed integer type (e.g., `int16_t` in C++ or `short` in C#).
3.  Divide by the **Scaling Factor**.

---

## 💻 Decoded Data Example (JSON)

Below is an example of what a decoded packet stream might look like on your main computer or dashboard:

```json
{
  "0x100": {
    "type": "Raw Accel",
    "ax": 0.045,      // Raw:  45
    "ay": -0.012,     // Raw: -12
    "az": 1.002       // Raw: 1002
  },
  "0x101": {
    "type": "Orientation",
    "pitch": 2.45,    // Raw: 245
    "roll": -1.10,    // Raw: -110
    "yaw": 145.30     // Raw: 14530
  },
  "0x102": {
    "type": "Altitude",
    "altitude_m": 12.50, // Raw: 1250
    "temp_c": 34.20      // Raw: 3420
  }
}
```
