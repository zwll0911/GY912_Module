# 🖥️ Web Dashboard (NAV Mission Control)

[🔙 **Back to Main README**](../README.md)

The **NAV Mission Control** dashboard is a browser-based ground control station for real-time telemetry monitoring, 3D orientation visualization, and flight data recording.

![Dashboard Preview](dashboard_preview.png)

---

## ⚡ Quick Start

1.  **Open** `firmware/index.html` in **Google Chrome** or **Microsoft Edge**.
2.  **Connect** the ESP32-S3 module to your computer via USB.
3.  Click the **🔌 CONNECT** button (top-right corner).
4.  Select **COM port** (e.g., `COM3`) from the browser dialog.
5.  The status bar will change to **"SYSTEM ONLINE • READY"**.

> [!IMPORTANT]
> **Browser Compatibility**: This dashboard uses the **Web Serial API**, which is only available in Chromium-based browsers (Chrome, Edge, Opera). It does **not** work in Firefox or Safari.

---

## 📊 Dashboard Panels

| Panel | Color | Data Displayed | Unit |
| :--- | :--- | :--- | :--- |
| **Accelerometer** | 🔴 Red | AX (Surge), AY (Sway), AZ (Heave) | m/s² |
| **Gyroscope** | 🔵 Blue | GX (Roll), GY (Pitch), GZ (Yaw) | deg/s |
| **Magnetometer** | 🟡 Yellow | MX (North), MY (East), MZ (Down) | µT |
| **Altimeter** | 🟢 Green | Altitude, Pressure | m / hPa |
| **3D Orientation** | 🟢 Green | Pitch, Roll, Yaw/Heading | degrees |

Each panel includes:
*   **Real-time numeric readouts** at the top.
*   **Live scrolling chart** (100-sample window) at the bottom.
*   **Core Temperature** is displayed in the header bar.

---

## 🧊 3D Orientation View

A **CSS 3D cube** at the bottom of the dashboard rotates in real-time to visualize the module's orientation.

*   **Pitch** and **Roll** are computed from accelerometer data.
*   **Yaw / Heading** is computed from magnetometer data.
*   The cube faces are labeled (FRONT, BACK, LEFT, RIGHT, TOP, BOT) for easy reference.

---

## 🎮 Simulation Mode

The dashboard includes a **Simulation Mode** for testing the UI without hardware connected.

### How to Use
1.  Open `index.html` in a browser (no hardware needed).
2.  Click the **🎮 SIMULATE** button.
3.  Status will change to **"SIMULATION MODE"** and charts will animate with fake sine-wave data.
4.  The 3D cube will rotate, charts will scroll, and the Flight Recorder works normally.
5.  Click **⏹ STOP SIM** to end simulation.

> [!TIP]
> Simulation Mode is great for **demos and presentations** where you don't have the hardware available.

---

## 🔴 Flight Recorder

The dashboard includes a built-in **Black Box** feature for data logging.

### How to Use
1.  Connect to the module (status must be **ONLINE** or **SIMULATION MODE**).
2.  Click the **🔴 REC** button to start recording.
3.  The button will pulse red while recording.
4.  Click **⬛ STOP** to end the recording.
5.  A **CSV file** will automatically download (e.g., `flight_log_2026-02-10T...csv`).

### CSV Format
```text
Timestamp, Ax, Ay, Az, Gx, Gy, Gz, Mx, My, Mz, Temp, Press, Alt
```

---

## 🔧 Serial Configuration

| Parameter | Value |
| :--- | :--- |
| **Baud Rate** | `115200` |
| **Data Format** | CSV (12 comma-separated values per line) |
| **Encoding** | UTF-8 |

### Expected Input Format
The firmware must output data in this exact CSV order:
```text
Ax,Ay,Az,Gx,Gy,Gz,Mx,My,Mz,Temp,Press,Alt
```

> [!TIP]
> **Offline Ready**: The `Chart.js` library is bundled locally in the `firmware/` folder. This dashboard works **100% offline** — no internet connection required at competition venues.
