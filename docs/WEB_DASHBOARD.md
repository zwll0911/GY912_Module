# 🖥️ Web Dashboard (NAV Mission Control)

[🔙 **Back to Main README**](../README.md)

The **NAV Mission Control** dashboard is a browser-based ground control station for real-time telemetry monitoring and flight data recording.

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

Each panel includes:
*   **Real-time numeric readouts** at the top.
*   **Live scrolling chart** (100-sample window) at the bottom.
*   **Core Temperature** is displayed in the header bar.

---

## 🔴 Flight Recorder

The dashboard includes a built-in **Black Box** feature for data logging.

### How to Use
1.  Connect to the module (status must be **ONLINE**).
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

> [!WARNING]
> **Internet Dependency**: The dashboard currently loads `Chart.js` from a CDN. If you are offline (e.g., at a competition venue), the charts will not render. Consider downloading `chart.js` locally for offline use.
