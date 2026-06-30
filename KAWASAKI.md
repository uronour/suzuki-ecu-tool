# Kawasaki KDS Support (2004 ZX-10R)

The Suzuki ECU Tool hardware is fully compatible with the Kawasaki Diagnostic System (KDS) protocol used in early EFI Kawasaki motorcycles.

## Protocol Comparison

| Parameter | Suzuki SDS | Kawasaki KDS |
|-----------|-----------|--------------|
| ECU Address | `0x12` | `0x11` |
| Tester Address | `0xF1` | `0xF1` |
| Initialization | Fast Init (`0x81`) | Fast Init (`0x81`) + Start Diag Session (`0x10 0x80`) |
| Baud Rate | 10,400 | 10,400 |

## Connection Pinout (ZX-10R 2004)

Use the **RS232 Header** on the BTT TFT35 V3.0:

1. **K-Line**: Pin 1 on the bike's diagnostic plug -> K-Line Driver -> **`PA3` (RX)** and **`PA2` (TX)**.
2. **Ground**: Ground pin on bike -> System Ground.
3. **Power**: +12V from bike -> Voltage Regulator -> System 5V/3.3V.

## How to use KDS Mode

1. Power on the Suzuki ECU Tool.
2. Open the **Android App**.
3. Go to **Settings**.
4. Change **Bike Brand** to **KAWASAKI**.
5. Tap **Connect**.
6. The STM32 will switch to KDS mode and start the initialization sequence for the Kawasaki ECU.

## KDS Supported PIDs

- **RPM**: `0x21 0x09`
- **Speed**: `0x21 0x0C`
- **TPS**: `0x21 0x04`
- **Engine Temp**: `0x21 0x06`
- **Gear Pos**: `0x21 0x0B`
