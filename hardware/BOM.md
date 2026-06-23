# Bill of Materials — Suzuki SDS K-Line Interface (TFT35 V3.0)

## Overview

Two small modular PCBs with a common 4-pin UART header (same pinout on both), so K-Line and BT can be swapped without rewiring.

### Common 4-pin UART Header Pinout (2.54mm pitch)

```
Pin 1: GND
Pin 2: +5V (from TFT35 5V rail)
Pin 3: TX (MCU transmit → peripheral RX)
Pin 4: RX (MCU receive ← peripheral TX)
```

## Board A — L9637D K-Line Interface

SMD components (0805 or 1206) except headers. PCB ~25×20mm.

| Qty | Part | Package | Purpose | UK Source | Cost |
|-----|------|---------|---------|-----------|------|
| 1 | L9637D | SOIC-8 | K-Line ISO 9141 transceiver | Farnell (2842898) / RS (5344111) / eBay UK | £3-5 |
| 1 | TVS (5.6V bidirectional) | SOD-123 (e.g. SMF5.0A) | K-Line overvoltage protection | Farnell / RS | £0.30 |
| 2 | 100nF ceramic | 0805 | L9637D decoupling (VCC + VS) | Farnell / RS | £0.10 |
| 1 | 1kΩ 1/4W | 0805 | K-Line pull-up to +12V | Farnell / RS | £0.05 |
| 1 | 2.2kΩ 1/4W | 0805 | Voltage divider (L9637D TX → STM32 RX) | Farnell / RS | £0.05 |
| 1 | 3.3kΩ 1/4W | 0805 | Voltage divider | Farnell / RS | £0.05 |
| 1 | 100Ω 1/4W | 0805 | K-Line current limit before TVS | Farnell / RS | £0.05 |
| 1 | 2-pin header | 2.54mm | K-Line +12V input (from bike) | eBay UK | £0.10 |
| 1 | 4-pin header (male) | 2.54mm | UART connection to TFT35 | eBay UK | £0.10 |

**Total Board A: ~£4**

## Board B — HC-05 Bluetooth Socket

SMD + through-hole. PCB ~25×20mm.

| Qty | Part | Package | Purpose | UK Source | Cost |
|-----|------|---------|---------|-----------|------|
| 1 | HC-05 BT module | 2.54mm pin header | Bluetooth SPP (USART1) | The Pi Hut / Amazon UK / eBay UK | £5-8 |
| 1 | 4-pin female header | 2.54mm | Socket for HC-05 | eBay UK | £0.10 |
| 1 | 4-pin header (male) | 2.54mm | UART connection to TFT35 | eBay UK | £0.10 |
| 1 | 100nF ceramic | 0805 | HC-05 decoupling | Farnell / RS | £0.05 |

**Total Board B: ~£6**

## System-Level Components

| Qty | Part | Purpose | UK Source | Cost |
|-----|------|---------|-----------|------|
| 1 | TFT35 V3.0 | Main controller, display, USART1/2/3 | 3D Jake / Amazon UK / eBay UK | £25-35 |
| 1 | ST-Link V2 clone | Firmware flashing/debug | Amazon UK / eBay UK | £3-5 |
| 1 | 6-pin Sumitomo connector | Bike diagnostic connector | See pinout/diagnostic_connector.md | £5-45 |
| 1 | USB-TTL adapter | Debug console (USART2) | The Pi Hut / Amazon UK | £4 |
| 1 | Mini breadboard + wires | Prototyping | The Pi Hut / Amazon UK | £3 |
| 1 | 1N4007 diode (optional) | Reverse polarity protection | Farnell / RS | £0.10 |
| 1 | 1A blade fuse + holder (optional) | Overcurrent protection | Farnell / RS / Amazon UK | £1 |

## Estimated Total

- **Minimum build** (TFT35 + Board A + BT Board B): **~£35-50**
- **With ST-Link + breadboard + USB-TTL**: **~£45-60**

## UK Sourcing Guide

### ICs & Components
| Store | URL | Best For |
|-------|-----|----------|
| **Farnell (element14)** | uk.farnell.com | L9637D, TVS, passives |
| **RS Components** | uk.rs-online.com | L9637D, TVS, passives |
| **eBay UK** | www.ebay.co.uk | HC-05, headers, ST-Link, cheapest passives |

### TFT35 V3.0
| Store | URL | Price |
|-------|-----|-------|
| **3D Jake** | www.3djake.uk | ~£30 |
| **Amazon UK** | www.amazon.co.uk (search "BIGTREETECH TFT35 V3.0") | £25-35 |

### HC-05 BT Module
| Store | URL | Price |
|-------|-----|-------|
| **The Pi Hut** | www.pihut.com | ~£7 |
| **Amazon UK** | www.amazon.co.uk | £5-8 |
| **eBay UK** | www.ebay.co.uk | £4-6 |

### ST-Link V2
| Store | URL | Price |
|-------|-----|-------|
| **Amazon UK** | www.amazon.co.uk (search "ST-Link V2") | £4-6 |
| **eBay UK** | www.ebay.co.uk | £3-5 |

### Diagnostic Connector
See `pinout/diagnostic_connector.md` for options (FTECU harness, AliExpress, ECU direct pins).

### General Electronics
| Store | URL | Best For |
|-------|-----|----------|
| **The Pi Hut** | www.pihut.com | HC-05, ST-Link, breadboards |
| **Amazon UK** | www.amazon.co.uk | TFT35, HC-05, ST-Link, everything |
| **eBay UK** | www.ebay.co.uk | Cheapest prices, variable quality |
