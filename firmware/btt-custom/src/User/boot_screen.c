#include <stdio.h>
#include <string.h>
#include "boot_screen.h"
#include "ili9488_gfx.h"
#include "os_timer.h"
#include "delay.h"
#include "lcd.h"

void Boot_Animation(void)
{
  LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, COLOR_BLACK);

  GFX_DrawStringScaled(60, 30, "Suzuki ECU Tool", RGB565(0, 180, 255), COLOR_BLACK, 3);
  GFX_DrawStringScaled(90, 80, "2004 GSX-R1000 SDS", RGB565(150, 150, 150), COLOR_BLACK, 2);

  GFX_DrawStringScaled(60, 120, "K-Line Diagnostic System", RGB565(0, 200, 80), COLOR_BLACK, 2);

  GFX_DrawStringScaled(170, 160, "Auto-Collaborate", RGB565(255, 180, 0), COLOR_BLACK, 1);

  GFX_DrawStringScaled(110, 190, "USR-C322 WiFi Bridge: 192.168.1.200", RGB565(100, 100, 100), COLOR_BLACK, 1);

  GFX_DrawStringScaled(155, 240, "Made by Uronour", RGB565(100, 100, 100), COLOR_BLACK, 1);

  int bar_x = 80, bar_y = 270, bar_w = 320, bar_h = 12;
  LCD_FillRect(bar_x, bar_y, bar_x + bar_w - 1, bar_y + bar_h - 1, COLOR_GRAY);

  for (int pct = 0; pct <= 100; pct += 4)
  {
    int fill = (bar_w * pct) / 100;
    LCD_FillRect(bar_x, bar_y, bar_x + fill - 1, bar_y + bar_h - 1, RGB565(0, 180, 255));
    Delay_ms(20);
  }

  Delay_ms(600);
}
