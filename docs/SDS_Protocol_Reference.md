# Suzuki SDS (KWP2000) Protocol Reference

**Last updated:** 2026-06-24  
**Target:** 2004 GSX-R1000 ECU  
**Based on:** aster94/KWP2000 library (GSX-R600 2011 tested), synfinatic/sv650sds, AIM SDS protocol docs, ISO 14230

---

## 1. Physical Layer

| Parameter | Value |
|-----------|-------|
| Protocol | KWP2000 (ISO 14230) over K-Line |
| Baud rate | 10,400 baud |
| Data format | 8N1 (8 data, no parity, 1 stop) |
| Voltage | 12V (K-Line), 5V (tester UART) |
| Interface IC | L9637D (or MC33660, MC33199) |
| Pull-up | 500-1k ohm on K-Line |
| Connector | Sumitomo 4-pin (under rider seat) |
| Dealer mode pin | PC0 (active high, optocoupler + 330 ohm) |

### K-Line Fast Init Timing

```
Tester: ________                   ___________
               |___25ms___|       |
                    ^          ^
               T_INIL(25ms)  T_WUP(50ms)
```

| Parameter | Value | Description |
|-----------|-------|-------------|
| T_IDLE | 1000ms | K-Line high before init |
| T_INIL | 25ms | Initialization low time |
| T_WUP | 50ms | Wake up pattern high |
| T_P1 | 10ms | Inter-byte, ECU response |
| T_P2_min | 25ms | Between request and ECU response |
| T_P2_max | 50ms | (negotiated, can be up to 89600ms) |
| T_P3_min | 55ms | Between ECU response and next request |
| T_P3_max | 2000ms | (negotiated) |
| T_P4_min | 5-10ms | Inter-byte, tester request |
| Keep-alive | 900ms | Send TesterPresent every ~900ms |

---

## 2. Frame Format

### 2.1 Byte order

All multi-byte values: **big-endian** (MSB first).

### 2.2 3-Byte Header (Request, typical)

```
[fmt|len] [target] [source] [SID + data...] [checksum]
```

- **fmt byte:** `0x80 | payload_len` (physical addressing, lower 6 bits = payload length)
- **target:** ECU = `0x12`
- **source:** Tester = `0xF1`
- **SID:** Service ID byte
- **data:** zero or more data bytes
- **checksum:** `sum(all_bytes_except_checksum) & 0xFF`

Max payload with 3-byte header = 63 bytes (limited by 6-bit length field).

### 2.3 4-Byte Header (Response, and some requests)

```
[0x80] [target] [source] [data_len] [SID + data...] [checksum]
```

- **fmt byte:** `0x80` (lower 6 bits = 0, meaning length follows as separate byte)
- **target:** Tester = `0xF1` (response) or ECU = `0x12` (request)
- **source:** ECU = `0x12` (response) or Tester = `0xF1` (request)
- **data_len:** Number of bytes after this byte (including SID, excluding checksum)
- **SID:** Service ID byte
- **checksum:** Same as 3-byte format

### 2.4 Checksum Calculation

```c
uint8_t checksum(uint8_t *buf, uint16_t len) {
    uint8_t sum = 0;
    for (uint16_t i = 0; i < len; i++)
        sum += buf[i];
    return sum;
}
```

Checksum is appended at end of frame. Verify by: `sum(frame[0 .. len-2]) == frame[len-1]`.

---

## 3. Service IDs (SIDs)

### 3.1 Diagnostic Services

| SID | Response SID | Name | Implemented |
|-----|-------------|------|-------------|
| `0x10` | `0x50` | DiagnosticSessionControl | NO (future) |
| `0x13` | `0x53` | ReadDiagnosticTroubleCodes | YES |
| `0x14` | `0x54` | ClearDiagnosticTroubleCodes | YES |
| `0x17` | `0x57` | ReadActiveTroubleCodes | NO |
| `0x18` | `0x58` | ReadDTCWithStatus | NO |
| `0x21` | `0x61` | ReadDataByLocalIdentifier | YES |
| `0x22` | `0x62` | ReadDataByCommonIdentifier | STUB |
| `0x23` | `0x63` | ReadMemoryByAddress | YES |
| `0x25` | `0x65` | StopDiagnosticSession / DealerMode? | YES |
| `0x27` | `0x67` | SecurityAccess | NO |
| `0x3D` | `0x7D` | WriteMemoryByAddress | YES |
| `0x3E` | `0x7E` | TesterPresent | YES |
| `0x34` | `0x74` | RequestDownload | YES |
| `0x35` | `0x75` | RequestUpload | NO (future) |
| `0x36` | `0x76` | TransferData | YES |
| `0x37` | `0x77` | RequestTransferExit | YES |
| `0x81` | `0xC1` | StartCommunication | YES |
| `0x82` | `0xC2` | StopCommunication | YES |
| `0x83` | `0xC3` | AccessTimingParameter | NO (future) |
| `0xBE` | `0x7E` | TesterPresent (alt) | NO |

### 3.2 Negative Response

When the ECU rejects a request:
```
[fmt] [target] [source] [len=0x02] [0x7F] [original_SID] [NRC] [CS]
```

Common NRCs:
| NRC | Name | Meaning |
|-----|------|---------|
| `0x11` | serviceNotSupported | Unknown SID |
| `0x12` | subFunctionNotSupported | Invalid sub-function |
| `0x13` | incorrectMessageLengthOrFormat | Wrong data length |
| `0x22` | conditionsNotCorrect | Wrong session / state |
| `0x31` | requestOutOfRange | Invalid address/parameters |
| `0x33` | securityAccessDenied | Not unlocked |
| `0x72` | generalProgrammingFailure | Flash write error |
| `0x78` | responsePending | ECU busy (send TesterPresent) |

---

## 4. Communication Sequence

### 4.1 Full Init Sequence

```
1. Tester: [0x81][0x12][0xF1][0x81][CS]             // StartCommunication (3-byte hdr)
   ECU:    [0x80][0xF1][0x12][0x03][0xC1][key1][key2][CS]  // 0xC1 + 2 keybytes

2. Tester: [0x80][0x12][0xF1][0x02][0x83][0x00][CS] // ATP read limits (4-byte hdr)
   ECU:    [0x80][0xF1][0x12][0x07][0xC3][0x00][P2m][P2M][P3m][P3M][P4m][CS]

3. Tester: [0x80][0x12][0xF1][0x02][0x83][0x02][CS] // ATP read current
   ECU:    [0x80][0xF1][0x12][0x07][0xC3][0x02][P2m][P2M][P3m][P3M][P4m][CS]

4. Normal operation (sensor poll, DTC, etc.)
```

**Note:** Our current `SDS_Init()` uses a simplified 2-step init (StartCommunication + ATP set to default). The full 3-step sequence above is from the KWP2000 library ECU emulator. We skip ATP negotiation and go straight to active mode. Both approaches have been tested to work.

### 4.2 Sensor Poll

```
Tester: [0x80][0x12][0xF1][0x02][0x21][0x08][CS]
ECU:    [0x80][0xF1][0x12][data_len][0x61][sensor_data...][CS]
```

### 4.3 Keep Alive

```
Tester: [0x80][0x12][0xF1][0x02][0x3E][0x01][CS]
ECU:    [0x80][0xF1][0x12][0x01][0x7E][CS]
```

### 4.4 DTC Read

```
Tester: [0x80][0x12][0xF1][0x01][0x13][CS]
ECU:    [0x80][0xF1][0x12][len][0x53][dtc_bytes...][CS]
```

### 4.5 DTC Clear

```
Tester: [0x80][0x12][0xF1][0x01][0x14][CS]
ECU:    [0x80][0xF1][0x12][0x01][0x54][CS]
```

### 4.6 Memory Read (0x23)

Request format: `[0x23][ALFID][addr_bytes...][size_bytes...]`
Response: `[0x63][data...]`

ALFID encoding:
| ALFID | Addr bytes | Size bytes | Use case |
|-------|-----------|-----------|----------|
| 0x11 | 1 | 1 | 8-bit MCU |
| 0x22 | 2 | 2 | 16-bit addr |
| 0x44 | 4 | 4 | 32-bit addr + size |
| 0x24 | 4 | 2 | 32-bit addr, max 64KB |
| 0x14 | 1 | 4 | 8-bit addr, 32-bit size |

**Calibration read window (typical Suzuki):** Addresses outside this range → NRC 0x31.

For 2004 GSX-R1000 (SH7058 MCU, 32-bit):
- Flash base: `0x000000` (512KB)
- Calibration: `0x008000 - 0x01FFFF` (~96KB)
- Boot: `0x000000 - 0x007FFF`

### 4.7 Bulk Download Sequence (0x34 → 0x36 → 0x37)

```
1. Tester: [0x34][DFI][ALFID][addr(4)][size(4)]
   ECU:    [0x74][LFI][maxBlockLen...]

2. Tester: [0x36][seq=1][data...]     // repeat with seq++, until all data sent
   ECU:    [0x76][seq]

3. Tester: [0x37]
   ECU:    [0x77]
```

**Note:** 0x34/0x36/0x37 is for **writing TO** the ECU (download from tester perspective). For reading, use 0x23 (ReadMemoryByAddress) or 0x35 (RequestUpload, for large reads).

---

## 5. Sensor Data Layout

### 5.1 Indexing Convention

Two reference implementations use different index bases:

- **KWP2000 library** (aster94, GSX-R600 2011): `_response[N]` indexes from raw frame byte 0
- **SDS reader** (synfinatic, SV650): `d[N]` indexes from first data byte after 4-byte header (= raw[N+4])

Our firmware matches the **KWP2000 library** convention (raw byte 0 indexing).

### 5.2 Known Offsets (raw byte index, KWP2000 library)

| Offset | Field | Formula | Tested |
|--------|-------|---------|--------|
| 16 | Speed | `d[16] * 2` (km/h raw * 2 = actual?) | YES (GSX-R600) |
| 17-18 | RPM | `d[17] * 10 + d[18] / 10` | YES |
| 19 | TPS | `125 * (d[19] - 55) / (256 - 55)` | YES |
| 20 | MAP | `d[20] * 4 * 0.136` (kPa) | YES |
| 21 | ECT | `(d[21] - 48) / 1.6` (°C) | YES |
| 22 | IAT | `(d[22] - 48) / 1.6` (°C) | YES |
| 24 | Battery | `d[24] * 100 / 126` (Volt*10) | YES |
| 25 | O2 sensor | `d[25]` (raw) | HINT |
| 26 | Gear | `d[26]` (0=neutral, 1-6) | YES |
| 27 | Baro | `d[27] * 4 * 0.136` (kPa) | HINT |
| 28 | Idle speed? | - | UNKNOWN |
| 29 | ISC/IAC steps | `d[29]` | HINT |
| 30 | Injector pulse (single) | `d[30]` (ms*10?) | HINT |
| 31-32 | Injector 0 | `d[31]*256 + d[32]` (ms*10) | HINT |
| 33-34 | Injector 1 | `d[33]*256 + d[34]` | HINT |
| 35-36 | Injector 2 | `d[35]*256 + d[36]` | HINT |
| 37-38 | Injector 3 | `d[37]*256 + d[38]` | HINT |
| 46 | STPS | `d[46] / 2.55` (%) | YES |
| 47 | Ignition angle | `d[47]` (deg) | HINT |
| 50 | Mode switch | `d[50]` | HINT |
| 51 | PAIR valve | `d[51]` | HINT |
| 52 | Clutch | `d[52] != 0` | YES |
| 53 | Neutral | `d[53] != 0` | HINT |

**Legend:** YES=confirmed working, HINT=mentioned in KWP2000 library comments, UNKNOWN=speculative

### 5.3 Response Frame Structure

```
Byte  0: 0x80 (format byte)
Byte  1: 0xF1 (target = tester)
Byte  2: 0x12 (source = ECU)
Byte  3: data_len (total following bytes including SID)
Byte  4: 0x61 (positive response SID for 0x21)
Byte  5: 0x08 (identifier echo)
Byte  6+: sensor data bytes
Last byte: checksum
```

**Important:** When the response uses 3-byte format (`0x81` or similar), there is NO length byte and NO identifier echo. The response SID and data follow immediately after the 3-byte header. Our firmware handles 4-byte format only.

### 5.4 Value Scaling Summary

| Value | Raw range | Conversion | Display |
|-------|-----------|------------|---------|
| RPM | 0-14000 | `d[17]*10 + d[18]/10` | `%lu rpm` |
| Speed | 0-255 | `d[16] * 2` (?) | `%lu km/h` |
| TPS | 0-255 | `125*(d[19]-55)/(256-55)` | `%lu %%` |
| MAP | 0-255 | `d[20]*4*0.136` | `%lu kPa` |
| ECT | 0-255 | `(d[21]-48)/1.6` | `%lu C` |
| IAT | 0-255 | `(d[22]-48)/1.6` | `%lu C` |
| Battery | 0-255 | `d[24]*100/126` | `%.1f V` |
| Gear | 0-6 | raw | `N,1-6` |
| STPS | 0-255 | `d[46]/2.55` | `%lu %%` |
| Clutch | 0/1 | raw | `IN/OUT` |

---

## 6. ECU Memory Map (2004 GSX-R1000 - ESTIMATED)

Based on typical SH7058 (32-bit Renesas SH-2) MCU layout used in Suzuki ECUs of this era:

| Region | Start | Size | Description |
|--------|-------|------|-------------|
| Boot/Vector | 0x000000 | 32KB | Reset vectors, bootloader |
| Main Flash | 0x008000 | 448KB | Main firmware (~96KB cal + ~352KB code) |
| EEPROM | 0x400000 | 2KB | Calibration constants (in external EEPROM) |
| RAM | 0xFF8000 | 12KB | Working RAM |

**Calibration region (likely readable via 0x23):** `0x008000` – `0x020000` (96KB)

**Note:** These are estimates. Exact memory map requires hardware testing or ECU disassembly.

---

## 7. Key References

### Source Code
- **aster94/Keyword-Protocol-2000:** https://github.com/aster94/Keyword-Protocol-2000
  - Full KWP2000 library for Arduino. Suzuki support (GSX-R600 2011 tested).
  - ECU Emulator in Python: `extras/ECU_Emulator/ECU_Emulator.py`
- **synfinatic/sv650sds:** https://github.com/synfinatic/sv650sds
  - SDS decoder + tools for SV650. Different data layout from GSX-R.
- **uronour/suzuki-ecu-tool:** This project

### Documentation
- **AIM Suzuki SDS Protocol:** `https://www.aim-sportline.com/download/ecu/stock/suzuki/Suzuki_SDS_Protocol_eng.pdf`
  - Lists 21 available channels for GSX-R (2005-2016)
- **AIM SDS 2 Protocol:** `https://www.aim-sportline.com/download/ecu/stock/suzuki/Suzuki_SDS_2_Protocol_eng.pdf`
  - Newer protocol for 2017+ models (different from ours)
- **ISO 14230-3:** Application layer (KWP2000 service definitions)
- **ISO 14229-1:** UDS application layer (for 0x23, 0x34, 0x36, 0x37)

### Community
- **ECU Hacking forum SDS thread:** `http://ecuhacking.activeboard.com/t22573776/sds-protocol/`
- **bmc-labs mini::bike:** K-Line to CAN converter with GSX-R1000 example
- **o5i/Datalogger:** https://github.com/o5i/Datalogger (Teensy + K-Line)

---

## 8. Project File Map

| File | Purpose |
|------|---------|
| `firmware/.../User/SDSProtocol.h` | Structs (SDS_Data, SDS_EcuInfo), protocol function declarations |
| `firmware/.../User/SDSProtocol.c` | Protocol implementation (frame, init, sensor poll, DTC, memory ops) |
| `firmware/.../User/kline_task.h/c` | K-Line UART init, fast init, byte send/receive, dealer mode |
| `firmware/.../User/gauge_ui.h` | Page enum (5 pages), action function declarations |
| `firmware/.../User/gauge_ui.c` | UI implementation (Live Data, Diagnostics, ECU Tools, Settings, About) |
| `emulator/sim_data.h` | Emulator data struct (all uint32_t, matches SDS_Data fields) |
| `emulator/sim_data.c` | ECU simulation (engine model, RPM/gears/sensors) |
| `emulator/main.c` | Emulator entry point, SDL event loop, key binding |
| `emulator/Makefile` | Build config with -DEMULATOR_BUILD flag |

---

## 9. Next Steps

1. Test physical K-Line communication with L9637D + HC-05 hardware
2. Verify sensor data offsets against real ECU
3. Discover exact calibration region address range using 0x23
4. Test 0x34/0x36/0x37 download sequence for reflash (HIGH RISK)
5. Add SecurityAccess (0x27) for protected regions
6. Add DiagnosticSessionControl (0x10) for programming session
