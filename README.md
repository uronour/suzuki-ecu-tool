# Suzuki & Kawasaki ECU Diagnostic Tool (v1.1.5)

A comprehensive K-Line (ISO 14230) diagnostic and dashboard system for 2004+ Suzuki (SDS) and Kawasaki (KDS) motorcycles. Built on the BigTreeTech TFT35 V3.0 hardware platform with a companion Android "Race" dashboard.

## 🚀 Key Features

### 1. Multi-Brand Support
- **Suzuki SDS**: Full diagnostics for 2004 GSX-R1000 and similar models.
- **Kawasaki KDS**: Support for 2004 ZX-10R and modern Kawasaki EFI models.
- **Real-time Protocol Switching**: Change brands via the Android app without reflashing.

### 2. Android "Race" Dashboard
- **3D Visuals**: Modern, high-performance gauges with neon glow, metallic bezels, and glass reflections.
- **10 Save Slots**: Create and store up to 10 unique dashboard layouts.
- **Auto-Connect**: Intelligent Bluetooth searching for specific ECU MAC address (`98:D3:34:90:C9:D9`).
- **Standalone Terminal**: Dedicated console page for manual KWP2000 command testing.

### 3. Hardware Diagnostic Suite
- **Smart-Sync Boot**: Hold the encoder knob at boot to trigger an automated K-Line loopback test and Bluetooth (HC-05) factory reset/configuration.
- **ST-Link Monitoring**: Real-time "X-Ray" debugging via Semihosting/SWD.

### 4. OTA Update System
- **Autonomous Server**: Python-based update server with background build detection.
- **One-Tap Update**: App automatically checks `suzuki-ecu.servebeer.com`, downloads new builds, and prompts for installation.

---

## 🛠 Hardware Configuration

### BigTreeTech TFT35 V3.0 (STM32F207)
| Function | Connector | Pins | Notes |
|----------|-----------|------|-------|
| **Bluetooth (HC-05)** | UART4 | `PC10` (TX), `PC11` (RX) | 115,200 Baud |
| **K-Line (ECU)** | RS232 (USART2) | `PA2` (TX), `PA3` (RX) | 10,400 Baud |
| **Reset Trigger** | Encoder | `PC6` | Hold at boot for diagnostics |
| **Dealer Mode** | GPIO | `PC0` | Logic high to activate |

### Physical Connections
- **K-Line Interface**: Requires an ISO 9141-2 / K-Line driver (e.g., L9637D or ISO Click).
- **Suzuki Plug**: White 6-pin connector. K-Line is typically White/Red.
- **Kawasaki Plug**: White 6-pin connector. K-Line is typically Pin 1.

---

## 💻 Software Architecture

### Firmware (`/firmware/btt-custom`)
- Built using **PlatformIO** (CMSIS framework).
- Uses non-blocking DMA serial handling for high-speed Bluetooth communication.
- Implements both SDS (Suzuki) and KDS (Kawasaki) initialization handshakes.

### Android App (`/android-app`)
- **Version**: 1.1.5 (Production)
- **Language**: Kotlin + Jetpack Compose/XML mix.
- **Communication**: Strict Bluetooth SPP (Serial Port Profile).

### Update Server (`/server`)
- Simple Python HTTP server with optimized APK streaming for mobile data.
- Automatically generates `version.json` whenever a new `app-debug.apk` is detected.

---

## 📂 Repository Structure

- `android-app/`: Full Android Studio project.
- `firmware/btt-custom/`: STM32 source code for the TFT35.
- `server/`: Python update server and manifest watcher.
- `docs/`: Pinouts, protocol logs, and hardware schematics.
- `emulator/`: C-based UI emulator for desktop development.

---

## 🛠 Development Commands

**Flash Firmware:**
```bash
pio run -t upload
```

**Run Update Server:**
```bash
python update_server.py 80
```

---

## ⚖️ License
© 2026 Uronour. All rights reserved.
For use with 2004 Suzuki GSX-R1000 and 2004 Kawasaki ZX-10R ECU projects.
