#include <stdio.h>
#include "boot_screen.h"
#include "ili9488_gfx.h"
#include "gauge_dial.h"
#include "os_timer.h"
#include "delay.h"
#include "lcd.h"

static void DrawGSXRLogo(int16_t cx, int16_t cy)
{
  int16_t x = cx - 72;
  int16_t y = cy - 18;
  GFX_DrawStringScaled(x, y, "GSX-R", RGB565(255, 40, 0), COLOR_BLACK, 3);
}

static void DrawGSXRLogoCentered(GaugeDial *d)
{
  int16_t x = d->cx - 72;
  int16_t y = d->cy - 45;
  GFX_DrawStringScaled(x, y, "GSX-R", RGB565(255, 40, 0), COLOR_BLACK, 3);
}

void Boot_Animation(void)
{
  GaugeDial bootTach = { 240, 160, 120, 135, 45, 0, 14000, 8 };
  DIAL_InitTable();

  uint32_t t0 = OS_GetTimeMs();

  LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, COLOR_BLACK);

  GFX_DrawStringScaled(80, 30, "Suzuki ECU Tool", RGB565(0, 180, 255), COLOR_BLACK, 3);
  GFX_DrawStringScaled(110, 75, "2004 GSX-R1000", RGB565(150, 150, 150), COLOR_BLACK, 2);

  DIAL_DrawArcSweep(&bootTach, 135, 45, RGB565(0, 180, 0));
  DIAL_DrawArcSweep(&bootTach, DIAL_AngleForValue(&bootTach, 10000), DIAL_AngleForValue(&bootTach, 12000), RGB565(200, 180, 0));
  DIAL_DrawArcSweep(&bootTach, DIAL_AngleForValue(&bootTach, 12000), 45, RGB565(200, 0, 0));
  DIAL_DrawTicks(&bootTach, 500, 2000, RGB565(120, 120, 120), RGB565(180, 180, 180));
  DIAL_DrawCenterHub(&bootTach, 12, RGB565(60, 60, 60));
  DrawGSXRLogoCentered(&bootTach);
  GFX_DrawStringScaled(bootTach.cx - 50, bootTach.cy + 35, "0 RPM", RGB565(200, 200, 200), COLOR_BLACK, 2);

  while (OS_GetTimeMs() - t0 < 1200)
  {
    uint32_t progress = OS_GetTimeMs() - t0;
    uint32_t sweepRpm;
    if (progress < 500)
    {
      sweepRpm = progress * 14000 / 500;
    }
    else if (progress < 1000)
    {
      sweepRpm = 14000 - (progress - 500) * 14000 / 500;
    }
    else
    {
      sweepRpm = 0;
    }

    DIAL_DrawNeedle(&bootTach, sweepRpm, RGB565(255, 80, 0), COLOR_BLACK, 0xFFFFFFFF);
    char rpmStr[16];
    snprintf(rpmStr, sizeof(rpmStr), "%u RPM", (unsigned)sweepRpm);
    LCD_FillRect(bootTach.cx - 80, bootTach.cy - 40, bootTach.cx + 80, bootTach.cy + 80, COLOR_BLACK);
    GFX_DrawStringScaled(bootTach.cx - 50, bootTach.cy + 35, rpmStr, RGB565(255, 255, 255), COLOR_BLACK, 2);
    DrawGSXRLogoCentered(&bootTach);
    Delay_ms(16);
  }

  DIAL_DrawNeedle(&bootTach, 0, RGB565(255, 80, 0), COLOR_BLACK, 0xFFFFFFFF);
  GFX_DrawStringScaled(140, 270, "Made by Uronour", RGB565(100, 100, 100), COLOR_BLACK, 1);
  Delay_ms(300);
}
