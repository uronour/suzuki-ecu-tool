#ifndef _GAUGE_UI_H_
#define _GAUGE_UI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define GAUGE_PAGE_COUNT 5

typedef enum {
  GAUGE_PAGE_LIVE_DATA = 0,
  GAUGE_PAGE_DIAGNOSTICS,
  GAUGE_PAGE_ECU_TOOLS,
  GAUGE_PAGE_SETTINGS,
  GAUGE_PAGE_ABOUT,
} GaugePage;

void Gauge_Init(void);
void Gauge_Update(void);
void Gauge_SetPage(GaugePage page);
GaugePage Gauge_GetPage(void);
void Gauge_NextPage(void);
void Gauge_PrevPage(void);
void Gauge_Press(void);
void Gauge_LongPress(void);
void Gauge_SetOrientation(uint8_t portrait);
void Gauge_ToggleOrientation(void);
uint8_t Gauge_IsSticky(void);
void Gauge_SetSticky(uint8_t sticky);
void Gauge_SelectUp(void);
void Gauge_SelectDown(void);
void Gauge_TouchAt(uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif

#endif
