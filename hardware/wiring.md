# Wiring Diagram — Suzuki SDS K-Line Interface (TFT35 V3.0)

## System Architecture

```
                    ┌──────────────────────────────────────────────┐
                    │              TFT35 V3.0                      │
                    │  ┌──────────────────────────────────────┐    │
                    │  │ STM32F207VCT6                        │    │
                    │  │                                      │    │
                    │  │ USART1 (PA9/PA10) ── WiFi port ──────┼───→ Board B (HC-05 BT)
                    │  │ USART2 (PD5/PD6) ── RS232 port ──────┼───→ USB-TTL debug
                    │  │ USART3 (PB10/PB11) ─ DIY port ───────┼───→ Board A (L9637D)
                    │  │                                      │    │
                    └──────────────────────────────────────────────┘
                                         │
                             4-pin UART header (common pinout)
                                         │
                         ┌───────────────┴───────────────┐
                         ▼                               ▼
                  ┌─────────────┐               ┌─────────────┐
                  │  Board A    │               │  Board B    │
                  │  L9637D     │               │  HC-05      │
                  │  K-Line     │               │  Bluetooth  │
                  └──────┬──────┘               └─────────────┘
                         │
                    ┌────┴────┐
                    │  Bike   │
                    │ 6-pin   │
                    │ Diag    │
                    │ Conn    │
                    └─────────┘
```

## Board A — L9637D K-Line Interface (USART3)

### L9637D Connections

```
L9637D (SOIC-8):
                    ┌──────┐
  TX  ────────── 1 ─┤      ├─ 8 ── GND
  RX  ────────── 2 ─┤      ├─ 7 ── +5V
  LO  ── NC ───── 3 ─┤      ├─ 6 ── NC
  K-I/O ──────── 4 ─┤      ├─ 5 ── +12V (from bike)
                    └──────┘
```

| L9637D Pin | Connect To |
|------------|------------|
| 1 (TX) | Voltage divider (2.2kΩ → 3.3kΩ to GND), output to STM32 USART3 RX (PB11) |
| 2 (RX) | STM32 USART3 TX (PB10) — 3.3V logic OK (L9637D VIH ~2.0V TTL) |
| 3 (LO) | NC (LIN mode — not used) |
| 4 (K-I/O) | Bike K-Line via 100Ω + TVS to GND, 1kΩ pull-up to +12V |
| 5 (VS) | +12V from bike (via reverse polarity protection diode 1N4007) |
| 6 (NC) | Not connected |
| 7 (VCC) | +5V (100nF to GND) |
| 8 (GND) | Ground plane |

### Level Shifting (L9637D TX → STM32 RX)

L9637D TX outputs 5V logic. STM32 is 3.3V. Simple voltage divider:

```
L9637D TX ──┬── 2.2kΩ ──┬── STM32 RX (PB11)
            │            │
            └── 3.3kΩ ───┴── GND
```

This gives ~3.3V at STM32 RX (5V × 3.3k/(2.2k+3.3k) = 3.0V, well within 3.3V logic levels).

### Input Protection

```
Bike +12V ──┬── 1A fuse ──┬── 1N4007 ──┬── L9637D VS (pin 5)
             │              │             │
             └──────────────┘              └── 100nF ── GND

Bike K-Line ── 100Ω ──┬── TVS 5.6V ── GND
                        │
                        └── 1kΩ ── +12V (pull-up)
```

### TFT35 Common UART Header Pinout (Board A connection)

| Pin | Signal | Connect To |
|-----|--------|------------|
| 1 | GND | L9637D GND (pin 8), system GND |
| 2 | +5V | L9637D VCC (pin 7) |
| 3 | TX (STM32→L9637D) | Voltage divider output → L9637D TX (pin 1) |
| 4 | RX (STM32←L9637D) | L9637D RX (pin 2) |

## Board B — HC-05 Bluetooth (USART1)

```
HC-05 Bluetooth Module:
                    ┌──────┐
  KEY  ────────── 1 ─┤      ├─ 6 ── NC
  VCC  ────────── 2 ─┤      ├─ 5 ── NC
  TX   ────────── 3 ─┤      ├─ 4 ── NC
  RX   ────────── 4 ─┤      │
                    └──────┘
```

HC-05 is 3.3V — connects directly to STM32 USART1 with no level shifting.

| HC-05 Pin | Connect To |
|-----------|------------|
| 1 (KEY) | NC (leave floating for normal mode) |
| 2 (VCC) | +5V (HC-05 onboard regulator handles 3.6-6V) |
| 3 (TX) | STM32 USART1 RX (PA10) — 3.3V, direct |
| 4 (RX) | STM32 USART1 TX (PA9) — 3.3V, direct |

### TFT35 Common UART Header Pinout (Board B connection)

| Pin | Signal | Connect To |
|-----|--------|------------|
| 1 | GND | HC-05 GND |
| 2 | +5V | HC-05 VCC |
| 3 | TX (STM32→HC-05) | HC-05 RX (pin 4) |
| 4 | RX (STM32←HC-05) | HC-05 TX (pin 3) |

## TFT35 V3.0 Port Connections

| TFT35 Port | Header Pins | Used For | Connected To |
|------------|-------------|----------|-------------|
| **WiFi** | 4-pin (USART1: PA9 TX, PA10 RX, 5V, GND) | BT module | Board B (HC-05) |
| **RS232** | 5-pin (USART2: PD5 TX, PD6 RX, 5V, GND, NC) | Debug console | USB-TTL adapter |
| **DIY** | 4-pin (USART3: PB10 TX, PB11 RX, 5V, GND) | K-Line | Board A (L9637D) |

### Connector Pinout (TFT35 side, all 2.54mm pitch)

```
WiFi Port (USART1):
  Pin 1: TX (PA9)
  Pin 2: RX (PA10)
  Pin 3: +5V
  Pin 4: GND

RS232 Port (USART2):
  Pin 1: TX (PD5)
  Pin 2: RX (PD6)
  Pin 3: +5V
  Pin 4: GND
  Pin 5: NC

DIY Port (USART3):
  Pin 1: TX (PB10)
  Pin 2: RX (PB11)
  Pin 3: +5V
  Pin 4: GND
```

## TFT35 Pin Allocation Summary

| STM32 Pin | Function | Connected To |
|-----------|----------|-------------|
| PA9 | USART1 TX | HC-05 RX |
| PA10 | USART1 RX | HC-05 TX |
| PD5 | USART2 TX | USB-TTL RX (debug) |
| PD6 | USART2 RX | USB-TTL TX (debug) |
| PB10 | USART3 TX | L9637D RX (via level shift? No, direct — L9637D RX is TTL) |
| PB11 | USART3 RX | L9637D TX (via voltage divider) |
| PC0 | Dealer mode output | 4N35 anode (optional, for dealer mode) |
| PA8 | Encoder A | Rotary encoder |
| PC9 | Encoder B | Rotary encoder |
| PC8 | Encoder button | Rotary encoder push |
| PC13 | XPT2046 TPEN | Touch IRQ |
| PE6/5/4/3 | XPT2046 SPI | Touch controller |

## Power Supply

- **TFT35 powered via**: USB-C or 12V input (board has onboard 5V regulator)
- **Board A powered via**: +5V and GND from TFT35 WiFi/DIY port (L9637D VCC) + separate +12V from bike for VS pin
- **Board B powered via**: +5V and GND from TFT35 WiFi port (HC-05 VCC)
- **Bike diagnostic connector**: Pin 2 (+12V), Pin 3 (GND), Pin 1 (K-Line), Pin 4 (DM)

**IMPORTANT**: L9637D pin 5 (VS) must connect to the bike's +12V (not the TFT35's 5V). The K-Line pull-up resistor also goes to bike +12V. The TFT35's 5V rail only powers the logic side of the L9637D (pin 7).
