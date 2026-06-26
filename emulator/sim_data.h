#ifndef _SIM_DATA_H_
#define _SIM_DATA_H_

#include <stdint.h>
#include <stdbool.h>

// Matches SDS_Data layout in SDSProtocol.h but all uint32_t for emulator compat
typedef struct {
    // Core sensors
    uint32_t rpm;
    uint32_t speed;           // km/h * 100
    uint32_t coolantTemp;     // Celsius
    uint32_t intakeAirTemp;   // Celsius
    uint32_t throttlePos;     // percent 0-100
    uint32_t batteryVolt;     // tenths of V: 142 = 14.2V
    uint32_t gearPos;
    uint32_t mapKpa;
    uint32_t o2Sensor;
    uint32_t stps;            // percent 0-100
    uint32_t injectorPulse;   // single approx
    uint32_t ignitionTiming;  // degrees BTDC
    uint32_t iacStep;         // ISC valve steps
    uint32_t fanOn;
    uint32_t sidestandDown;
    uint32_t clutchIn;

    // Extended sensors
    uint32_t baroKpa;         // barometric pressure
    uint32_t injectorPW[4];   // individual injectors *10 (ms*10)
    uint32_t pairValve;       // PAIR solenoid
    uint32_t neutral;         // neutral switch
} SDSData;

// ECU info matching SDS_EcuInfo
typedef struct {
    uint32_t ecuRawId[4];
    uint8_t  vin[17];
    uint8_t  vinLen;
    uint32_t flashSize;
    uint32_t calOffset;
    uint32_t calSize;
} SDSEcuInfo;

extern SDSData g_sdsData;
extern SDSEcuInfo g_sdsEcuInfo;
extern uint8_t  g_klineConnected;
extern uint8_t  g_dealerMode;

void SimData_Init(void);
void SimData_Update(void);
void SimData_ShiftUp(void);
void SimData_ShiftDown(void);
void SimData_SetThrottle(uint8_t held);
void SimData_ToggleClutch(void);

#endif