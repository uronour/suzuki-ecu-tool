# OTA Update Server Setup

This server handles automatic updates for the Suzuki ECU Tool Android App.

## Prerequisites
1. **Python 3.x** installed on the host PC.
2. **Port Forwarding**: Forward Port 80 (TCP) to the host machine's local IP.
3. **No-IP**: Configure a dynamic DNS (e.g., `suzuki-ecu.servebeer.com`) to track your public IP.

## Server Operation

The server is designed to be fully autonomous. 

1. **Auto-Manifest**: A background thread watches the `/update` folder. 
2. **New Builds**: Whenever you drop a new `app-debug.apk` into the `/update` folder, the server automatically updates `version.json` with a timestamp-based version code.
3. **Optimized Streaming**: The server streams the APK in small chunks to ensure stability over mobile data (4G/5G).

## How to Start

Run the batch file in the project root:
`E:\suzuki-ecu-tool\run_update_server.bat`

To run on Windows Startup:
1. Press `Win + R`.
2. Type `shell:startup`.
3. Create a shortcut to the `.bat` file in that folder.
