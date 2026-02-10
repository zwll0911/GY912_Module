# CAN Bus Protocol Specification

This document defines the communication protocol for the Industrial AHRS Navigation Module (V3.0). The module broadcasts telemetry data at **500kbps**.

## Message IDs & Payload Definition

| CAN ID | Description | Data Type | Payload Structure (8 Bytes) | Scaling Factor |
|--------|-------------|-----------|-----------------------------|----------------|
| **0x100** | Raw Accelerometer | `int16_t` | Byte 0-1: Ax<br>Byte 2-3: Ay<br>Byte 4-5: Az<br>Byte 6-7: Reserved | Divide by 1000.0 for G-force |
| **0x101** | Orientation (Euler) | `int16_t` | Byte 0-1: Pitch<br>Byte 2-3: Roll<br>Byte 4-5: Yaw<br>Byte 6-7: Reserved | Divide by 100.0 for Degrees |
| **0x102** | Altitude & Baro | `int16_t` | Byte 0-1: Altitude<br>Byte 2-3: Temperature<br>Byte 4-7: Reserved | Alt / 100.0 (m)<br>Temp / 100.0 (°C) |

## Data Interpretation Details

- **Endianness**: Little Endian (Standard for x86/ARM).
- **Signed Integers**: All values are signed `int16_t`. A value of `-1500` corresponds to `-15.00` units after scaling.

## Decoded Examples (JSON)

Below is an example of what a decoded packet stream might look like on the main computer:

```json
{
  "0x100": {
    "type": "Raw Accel",
    "ax": 0.045,
    "ay": -0.012,
    "az": 1.002
  },
  "0x101": {
    "type": "Orientation",
    "pitch": 2.45,
    "roll": -1.10,
    "yaw": 145.30
  },
  "0x102": {
    "type": "Altitude",
    "altitude_m": 12.50,
    "temp_c": 34.20
  }
}
```
