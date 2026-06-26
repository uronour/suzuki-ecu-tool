#include "touch_calib.h"
#include "xpt2046.h"
#include "ili9488_gfx.h"
#include "os_timer.h"
#include "stm32f2_f4xx/HAL_Flash.h"
#include <string.h>

// Default calibration for BTT TFT35 V3.0 (480x320 landscape)
#define A_DEFAULT   120
#define B_DEFAULT   0
#define C_DEFAULT   (-2480)
#define D_DEFAULT   0
#define E_DEFAULT   160
#define F_DEFAULT   (-3100)
#define K_DEFAULT   4096

int32_t g_tpCal[7] = { A_DEFAULT, B_DEFAULT, C_DEFAULT, D_DEFAULT, E_DEFAULT, F_DEFAULT, K_DEFAULT };
uint8_t g_tpCalibrated = 0;

void TP_GetCoordinates(uint16_t *x, uint16_t *y)
{
  uint16_t raw_x = XPT2046_Repeated_Compare_AD(CMD_RDX);
  uint16_t raw_y = XPT2046_Repeated_Compare_AD(CMD_RDY);

  int32_t tx = (g_tpCal[0] * (int32_t)raw_x + g_tpCal[1] * (int32_t)raw_y + g_tpCal[2]) / g_tpCal[6];
  int32_t ty = (g_tpCal[3] * (int32_t)raw_x + g_tpCal[4] * (int32_t)raw_y + g_tpCal[5]) / g_tpCal[6];

  if (tx < 0) tx = 0;
  if (tx >= g_lcdWidth) tx = g_lcdWidth - 1;
  if (ty < 0) ty = 0;
  if (ty >= g_lcdHeight) ty = g_lcdHeight - 1;

  *x = (uint16_t)tx;
  *y = (uint16_t)ty;
}

static uint8_t WaitTouchDown(void)
{
  uint32_t start = OS_GetTimeMs();
  while ((OS_GetTimeMs() - start) < 60000)
  {
    if (XPT2046_Read_Pen() == 0)
    {
      uint32_t stable = OS_GetTimeMs();
      while ((OS_GetTimeMs() - stable) < 50)
      {
        if (XPT2046_Read_Pen() != 0)
          stable = OS_GetTimeMs();
      }
      return 1;
    }
  }
  return 0;
}

static void WaitTouchUp(void)
{
  uint32_t start = OS_GetTimeMs();
  while ((OS_GetTimeMs() - start) < 60000)
  {
    if (XPT2046_Read_Pen() != 0)
      return;
  }
}

uint8_t TP_Calibrate(void)
{
  uint16_t lcdX[3] = { 40, g_lcdWidth - 40, g_lcdWidth - 40 };
  uint16_t lcdY[3] = { 40, 40, g_lcdHeight - 40 };
  uint16_t tpX[3], tpY[3];
  uint16_t bg = RGB565(18, 18, 18);
  uint16_t red = RGB565(220, 40, 40);
  uint16_t white = RGB565(220, 220, 220);
  uint16_t green = RGB565(0, 200, 80);

  uint8_t retry;
  for (retry = 0; retry < 3; retry++)
  {
    LCD_Fill(0, 0, g_lcdWidth - 1, g_lcdHeight - 1, bg);
    GFX_DrawStringCenterScaled(20, "Touch Calibration", white, bg, 2);
    GFX_DrawStringCenterScaled(45, "Press the center of each red dot", RGB565(140, 140, 140), bg, 1);

    for (uint8_t pt = 0; pt < 3; pt++)
    {
      LCD_FillCircle(lcdX[pt], lcdY[pt], 6, red);
      for (uint8_t j = 0; j < 12; j++)
      {
        LCD_DrawPixel(lcdX[pt] + j, lcdY[pt], red);
        if (j <= lcdX[pt]) LCD_DrawPixel(lcdX[pt] - j, lcdY[pt], red);
        LCD_DrawPixel(lcdX[pt], lcdY[pt] + j, red);
        if (j <= lcdY[pt]) LCD_DrawPixel(lcdX[pt], lcdY[pt] - j, red);
      }

      if (!WaitTouchDown())
        return 0;

      tpX[pt] = XPT2046_Repeated_Compare_AD(CMD_RDX);
      tpY[pt] = XPT2046_Repeated_Compare_AD(CMD_RDY);

      WaitTouchUp();
    }

    int32_t k = ((int32_t)tpX[0] - (int32_t)tpX[2]) * ((int32_t)tpY[1] - (int32_t)tpY[2])
              - ((int32_t)tpX[1] - (int32_t)tpX[2]) * ((int32_t)tpY[0] - (int32_t)tpY[2]);

    if (k == 0) continue;

    g_tpCal[6] = k;
    g_tpCal[0] = ((int32_t)lcdX[0] - (int32_t)lcdX[2]) * ((int32_t)tpY[1] - (int32_t)tpY[2])
               - ((int32_t)lcdX[1] - (int32_t)lcdX[2]) * ((int32_t)tpY[0] - (int32_t)tpY[2]);
    g_tpCal[1] = ((int32_t)tpX[0] - (int32_t)tpX[2]) * ((int32_t)lcdX[1] - (int32_t)lcdX[2])
               - ((int32_t)lcdX[0] - (int32_t)lcdX[2]) * ((int32_t)tpX[1] - (int32_t)tpX[2]);
    g_tpCal[2] = (int32_t)tpY[0] * ((int32_t)tpX[2] * (int32_t)lcdX[1] - (int32_t)tpX[1] * (int32_t)lcdX[2])
               + (int32_t)tpY[1] * ((int32_t)tpX[0] * (int32_t)lcdX[2] - (int32_t)tpX[2] * (int32_t)lcdX[0])
               + (int32_t)tpY[2] * ((int32_t)tpX[1] * (int32_t)lcdX[0] - (int32_t)tpX[0] * (int32_t)lcdX[1]);
    g_tpCal[3] = ((int32_t)lcdY[0] - (int32_t)lcdY[2]) * ((int32_t)tpY[1] - (int32_t)tpY[2])
               - ((int32_t)lcdY[1] - (int32_t)lcdY[2]) * ((int32_t)tpY[0] - (int32_t)tpY[2]);
    g_tpCal[4] = ((int32_t)tpX[0] - (int32_t)tpX[2]) * ((int32_t)lcdY[1] - (int32_t)lcdY[2])
               - ((int32_t)lcdY[0] - (int32_t)lcdY[2]) * ((int32_t)tpX[1] - (int32_t)tpX[2]);
    g_tpCal[5] = (int32_t)tpY[0] * ((int32_t)tpX[2] * (int32_t)lcdY[1] - (int32_t)tpX[1] * (int32_t)lcdY[2])
               + (int32_t)tpY[1] * ((int32_t)tpX[0] * (int32_t)lcdY[2] - (int32_t)tpX[2] * (int32_t)lcdY[0])
               + (int32_t)tpY[2] * ((int32_t)tpX[1] * (int32_t)lcdY[0] - (int32_t)tpX[0] * (int32_t)lcdY[1]);

    // Verify: show center dot, ask user to tap it
    uint16_t cx = g_lcdWidth / 2;
    uint16_t cy = g_lcdHeight / 2;
    LCD_FillCircle(cx, cy, 4, green);
    GFX_DrawStringCenterScaled(g_lcdHeight - 40, "Tap the green dot", white, bg, 2);

    if (WaitTouchDown())
    {
      uint16_t vx, vy;
      TP_GetCoordinates(&vx, &vy);
      int16_t dx = (int16_t)vx - (int16_t)cx;
      int16_t dy = (int16_t)vy - (int16_t)cy;
      if (dx < 0) dx = -dx;
      if (dy < 0) dy = -dy;

      if (dx < 40 && dy < 40)
      {
        WaitTouchUp();
        g_tpCalibrated = 1;
        TP_SaveCalibration();
        return 1;
      }
    }
    WaitTouchUp();
    // Retry if verification failed
  }

  TP_SetDefaults();
  return 0;
}

void TP_SetDefaults(void)
{
  g_tpCal[0] = A_DEFAULT;
  g_tpCal[1] = B_DEFAULT;
  g_tpCal[2] = C_DEFAULT;
  g_tpCal[3] = D_DEFAULT;
  g_tpCal[4] = E_DEFAULT;
  g_tpCal[5] = F_DEFAULT;
  g_tpCal[6] = K_DEFAULT;
  g_tpCalibrated = 0;
}

#define TSC_SIGN 0x20200512  // matches BTT convention

static void wordToByte(uint32_t word, uint8_t *bytes)
{
  bytes[0] = (word >> 24) & 0xFF;
  bytes[1] = (word >> 16) & 0xFF;
  bytes[2] = (word >> 8) & 0xFF;
  bytes[3] = word & 0xFF;
}

static uint32_t byteToWord(const uint8_t *bytes)
{
  return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16)
       | ((uint32_t)bytes[2] << 8) | bytes[3];
}

void TP_SaveCalibration(void)
{
  // Store calibration parameters to flash (32 bytes: sign + 7 params)
  // Uses the same sector as BTT firmware (0x08004000)
  uint8_t data[32];
  wordToByte(TSC_SIGN, data);
  for (int i = 0; i < 7; i++)
    wordToByte((uint32_t)g_tpCal[i], data + 4 + i * 4);
  HAL_FlashWrite(data, sizeof(data));
}

void TP_LoadCalibration(void)
{
  uint8_t data[32];
  HAL_FlashRead(data, sizeof(data));
  if (byteToWord(data) == TSC_SIGN)
  {
    for (int i = 0; i < 7; i++)
      g_tpCal[i] = (int32_t)byteToWord(data + 4 + i * 4);
    g_tpCalibrated = 1;
  }
  else
  {
    TP_SetDefaults();
  }
}
