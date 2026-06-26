#ifndef _TOUCH_CALIB_H_
#define _TOUCH_CALIB_H_

#include <stdint.h>
#include <stdbool.h>

#define CMD_RDX  0xD0
#define CMD_RDY  0x90

extern int32_t g_tpCal[7];
extern uint8_t g_tpCalibrated;

void TP_GetCoordinates(uint16_t *x, uint16_t *y);
bool TP_IsPressed(void);
uint8_t TP_Calibrate(void);
void TP_SaveCalibration(void);
void TP_LoadCalibration(void);
void TP_SetDefaults(void);

#endif
