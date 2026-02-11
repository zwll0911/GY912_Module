# 🖥️ Web Dashboard (NAV PANEL v5.1)

[🔙 **Back to Main README**](../README.md)

The **NAV PANEL** is a browser-based ground control station for real-time telemetry monitoring and 3D orientation visualization, featuring a **Cyberpunk Neon Purple & Cyan** theme designed for high contrast in competition environments.

---

## ⚡ Quick Start

1.  **Open** `firmware/index.html` in **Google Chrome** or **Microsoft Edge**.
2.  **Connect** the ESP32-S3 module to your computer via USB.
3.  Click the **CONNECT** button (top-right corner).
4.  Select the **COM port** (e.g., `COM3`) from the browser dialog.
5.  Status indicator changes from `● OFFLINE` to `● ONLINE` (cyan).

> [!IMPORTANT]
> **Browser Compatibility**: This dashboard uses the **Web Serial API**, which is only available in Chromium-based browsers (Chrome, Edge, Opera). It does **not** work in Firefox or Safari.

---

## 📊 Dashboard Layout

The dashboard uses a responsive CSS Grid that fills 100% of the viewport without scrolling.

### Panel Overview

| Panel | Position | Data Displayed |
| :--- | :--- | :--- |
| **Primary Navigation** | Left column | Heading (°), Relative Heading (°), Pitch, Roll |
| **Spatial Awareness** | Center | Real-time 3D Cube (mirrors physical Yaw/Pitch/Roll) |
| **Environment** | Right column | Altitude (m), Temperature (°C), Pressure (hPa) |
| **System Health** | Right column (bottom) | CPU Load status, CAN Bus status (1 MBPS) |
| **Stability Graphs** | Bottom row (3 panels) | Yaw, Pitch, and Roll line charts (40-sample window) |

---

## 🧊 3D Orientation View

A **CSS 3D cube** rotates in real-time to mirror the robot's physical orientation.

*   **Yaw** → `rotateY`
*   **Pitch** → `rotateX` (inverted)
*   **Roll** → `rotateZ` (inverted)
*   Faces are labeled: **FRONT** (cyan highlight), BACK, LEFT, RIGHT, TOP, BOTTOM.

---

## 🎯 Tare Zero Point

The **TARE ZERO POINT** button captures the current Yaw heading and subtracts it from all future readings, allowing the driver to:
*   Instantly reset the front-facing heading to **0°**
*   Monitor relative heading changes from any arbitrary starting position

The **RELATIVE** readout in the Primary Navigation panel shows the tared value.

---

## 📈 Stability Graphs

Three separate **Chart.js** line graphs display:

| Graph | Color | Data Source |
| :--- | :--- | :--- |
| **Yaw Stability** | 🔵 Cyan | Tared heading (yaw − tare offset) |
| **Pitch** | 🟣 Purple | Pitch angle from DMP |
| **Roll** | 🔴 Pink | Roll angle from DMP |

Each graph shows a **40-sample rolling window** with no animation for maximum performance.

---

## 🔧 Serial Configuration

| Parameter | Value |
| :--- | :--- |
| **Baud Rate** | `115200` |
| **Data Format** | CSV (13 comma-separated values per line) |
| **Encoding** | UTF-8 |

### Expected Input Format
The firmware outputs data in this CSV order:
```text
0,0,0,Pitch,Roll,Yaw,Yaw,0,0,0,Temp,Pressure,Altitude
```
Fields at indices 0-2 and 7-9 are reserved (sent as `0.0`).

> [!TIP]
> **Offline Ready**: The `Chart.js` library is bundled locally in the `firmware/` folder. This dashboard works **100% offline** — no internet connection required at competition venues.
