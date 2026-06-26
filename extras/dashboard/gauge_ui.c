#include <stdio.h>
#include <string.h>
#include "gauge_ui.h"
#include "gauge_dial.h"
#include "ili9488_gfx.h"
#include "SDSProtocol.h"
#include "kline_task.h"
#include "bt_stream.h"
#include "sd_log.h"
#include "font_8x13.h"
#include "os_timer.h"

#define TACH_CX      140
#define TACH_CY      200
#define TACH_R       130
#define TACH_START   135
#define TACH_END     45
#define TACH_ARC_W   8
#define TACH_MAX     14000
#define TACH_REDLINE 12000
#define TACH_YLIMIT  10000

#define DIG_X        310
#define DIG_LABEL_Y  95
#define DIG_VAL_Y    120
#define DIG_STEP     65
#define DIG_SCALE    3

#define PAGE_FOOTER_Y (LCD_HEIGHT - FONT_H - 4)

#define SPEEDO_CX    340
#define SPEEDO_CY    200
#define SPEEDO_R     130
#define SPEEDO_START 135
#define SPEEDO_END   45
#define SPEEDO_ARC_W 8
#define SPEEDO_MAX   300

static GaugePage g_currentPage = GAUGE_PAGE_DASHBOARD;
static DashStyle g_dashStyle = DASH_STYLE_OEM;
static uint32_t g_lastUpdate = 0;
static uint32_t g_prevNeedleVal = 0xFFFFFFFF;
static uint32_t g_prevSpeedVal = 0xFFFFFFFF;
static uint32_t g_prevDig[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static uint8_t g_detailMode = 0;
static char g_prevRpmStr[16] = {0};
static uint32_t g_prevDetailVals[8] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static uint8_t g_detailDrawn = 0;
static uint8_t g_prevGear = 0xFF;

static uint8_t g_pageDirty = 1;
static const char *gearStr[] = { "N", "1", "2", "3", "4", "5", "6" };
static uint32_t g_prevSensors[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static uint16_t g_prevDtcCount = 0xFFFF;
static uint8_t g_prevSettings[4] = {0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t g_aboutDrawn = 0;

typedef struct {
    uint16_t bg;
    uint16_t arc1;
    uint16_t arc2;
    uint16_t arc3;
    uint16_t tickMajor;
    uint16_t tickMinor;
    uint16_t label;
    uint16_t needle;
    uint16_t hub;
    uint16_t digit;
    uint16_t logo;
    uint16_t gear;
    uint16_t detailBg;
    uint16_t detailLabel;
    uint16_t accent;
    uint32_t tickEvery;
    uint32_t labelEvery;
    uint8_t  digitScale;
    uint8_t  needleWidth;
    uint8_t  singleArc;
} StyleConfig;

static const StyleConfig g_styles[DASH_STYLE_COUNT] = {
    // DASH_STYLE_OEM
    {
        .bg = RGB565(30, 30, 30),
        .arc1 = RGB565(0, 180, 0),
        .arc2 = RGB565(200, 180, 0),
        .arc3 = RGB565(200, 0, 0),
        .tickMajor = RGB565(220, 220, 220),
        .tickMinor = RGB565(180, 180, 180),
        .label = RGB565(220, 220, 220),
        .needle = RGB565(255, 80, 0),
        .hub = RGB565(80, 80, 80),
        .digit = COLOR_WHITE,
        .logo = RGB565(255, 40, 0),
        .gear = RGB565(255, 200, 0),
        .detailBg = RGB565(20, 20, 20),
        .detailLabel = RGB565(100, 100, 100),
        .accent = COLOR_WHITE,
        .tickEvery = 500,
        .labelEvery = 2000,
        .digitScale = 3,
        .needleWidth = 9,
        .singleArc = 0,
    },
    // DASH_STYLE_RACE
    {
        .bg = RGB565(5, 5, 5),
        .arc1 = RGB565(220, 60, 0),
        .arc2 = RGB565(220, 60, 0),
        .arc3 = RGB565(255, 0, 0),
        .tickMajor = RGB565(180, 180, 180),
        .tickMinor = RGB565(100, 100, 100),
        .label = RGB565(220, 220, 220),
        .needle = RGB565(255, 30, 0),
        .hub = RGB565(60, 60, 60),
        .digit = RGB565(255, 200, 0),
        .logo = RGB565(255, 60, 0),
        .gear = RGB565(255, 200, 0),
        .detailBg = RGB565(10, 10, 10),
        .detailLabel = RGB565(120, 120, 120),
        .accent = RGB565(255, 100, 0),
        .tickEvery = 500,
        .labelEvery = 2000,
        .digitScale = 4,
        .needleWidth = 11,
        .singleArc = 1,
    },
    // DASH_STYLE_FLAT
    {
        .bg = RGB565(22, 22, 22),
        .arc1 = RGB565(200, 200, 200),
        .arc2 = RGB565(200, 200, 200),
        .arc3 = RGB565(255, 80, 80),
        .tickMajor = RGB565(160, 160, 160),
        .tickMinor = RGB565(90, 90, 90),
        .label = RGB565(180, 180, 180),
        .needle = RGB565(0, 200, 255),
        .hub = RGB565(40, 40, 40),
        .digit = RGB565(180, 180, 180),
        .logo = RGB565(80, 80, 80),
        .gear = RGB565(0, 200, 255),
        .detailBg = RGB565(25, 25, 25),
        .detailLabel = RGB565(100, 100, 100),
        .accent = RGB565(0, 200, 255),
        .tickEvery = 1000,
        .labelEvery = 2000,
        .digitScale = 3,
        .needleWidth = 7,
        .singleArc = 1,
    },
    // DASH_STYLE_NEO_RETRO
    {
        .bg = RGB565(12, 8, 25),
        .arc1 = RGB565(0, 200, 255),
        .arc2 = RGB565(255, 0, 128),
        .arc3 = RGB565(180, 0, 200),
        .tickMajor = RGB565(180, 100, 220),
        .tickMinor = RGB565(60, 30, 100),
        .label = RGB565(180, 100, 220),
        .needle = RGB565(0, 220, 255),
        .hub = RGB565(40, 20, 70),
        .digit = RGB565(255, 0, 128),
        .logo = RGB565(0, 220, 255),
        .gear = RGB565(255, 0, 128),
        .detailBg = RGB565(18, 12, 30),
        .detailLabel = RGB565(120, 60, 160),
        .accent = RGB565(255, 0, 128),
        .tickEvery = 500,
        .labelEvery = 2000,
        .digitScale = 3,
        .needleWidth = 9,
        .singleArc = 0,
    },
};

static inline const StyleConfig *GetStyle(void)
{
    return &g_styles[g_dashStyle];
}

static GaugeDial g_tach = {
  .cx = TACH_CX, .cy = TACH_CY,
  .radius = TACH_R,
  .startAngle = TACH_START, .endAngle = TACH_END,
  .minVal = 0, .maxVal = TACH_MAX,
  .arcWidth = TACH_ARC_W
};

static GaugeDial g_speedo = {
  .cx = SPEEDO_CX, .cy = SPEEDO_CY,
  .radius = SPEEDO_R,
  .startAngle = SPEEDO_START, .endAngle = SPEEDO_END,
  .minVal = 0, .maxVal = SPEEDO_MAX,
  .arcWidth = SPEEDO_ARC_W
};

static void DigLabel(int idx, const char *label)
{
  const StyleConfig *s = GetStyle();
  int16_t y = DIG_LABEL_Y + idx * DIG_STEP;
  GFX_DrawString(DIG_X, y, label, s->label, s->bg);
}

static void DigValue(int idx, const char *text, uint16_t color)
{
  const StyleConfig *s = GetStyle();
  int16_t y = DIG_VAL_Y + idx * DIG_STEP;
  int16_t w = strlen(text) * FONT_STEP * s->digitScale;
  int16_t h = FONT_H * s->digitScale;
  LCD_FillRect(DIG_X - 4, y - 4, DIG_X + w + 4, y + h + 4, s->bg);
  GFX_DrawStringScaled(DIG_X, y, text, color, s->bg, s->digitScale);
  LCD_DrawHLine(DIG_X - 2, DIG_X + w + 2, y + h + 2, s->accent);
}

static void DigValueU32(int idx, uint32_t val, const char *unit, uint16_t color)
{
  char buf[24];
  snprintf(buf, sizeof(buf), "%lu %s", (unsigned long)val, unit ? unit : "");
  DigValue(idx, buf, color);
}

static void DrawTachoFace(void)
{
  const StyleConfig *s = GetStyle();
  DIAL_DrawGaugeRing(&g_tach, RGB565(70, 70, 70), RGB565(40, 40, 40), RGB565(12, 12, 12));
  if (s->singleArc)
  {
    DIAL_DrawArcSweep(&g_tach, TACH_START, TACH_END, s->arc1);
    DIAL_DrawArcSweep(&g_tach, DIAL_AngleForValue(&g_tach, TACH_REDLINE), TACH_END, s->arc3);
  }
  else
  {
    DIAL_DrawArcSweep(&g_tach, TACH_START, DIAL_AngleForValue(&g_tach, TACH_YLIMIT), s->arc1);
    DIAL_DrawArcSweep(&g_tach, DIAL_AngleForValue(&g_tach, TACH_YLIMIT), DIAL_AngleForValue(&g_tach, TACH_REDLINE), s->arc2);
    DIAL_DrawArcSweep(&g_tach, DIAL_AngleForValue(&g_tach, TACH_REDLINE), TACH_END, s->arc3);
  }
  DIAL_DrawTicks(&g_tach, s->tickEvery, s->labelEvery, s->tickMinor, s->tickMajor);
  DIAL_DrawCenterHub(&g_tach, 12, s->hub);
  GFX_DrawStringScaled(g_tach.cx - 36, g_tach.cy - 9, "GSX-R", s->logo, s->bg, 2);
}

static void DrawSpeedoFace(void)
{
  const StyleConfig *s = GetStyle();
  DIAL_DrawGaugeRing(&g_speedo, RGB565(70, 70, 70), RGB565(40, 40, 40), RGB565(12, 12, 12));
  if (s->singleArc)
  {
    DIAL_DrawArcSweep(&g_speedo, SPEEDO_START, SPEEDO_END, s->arc1);
  }
  else
  {
    DIAL_DrawArcSweep(&g_speedo, SPEEDO_START, DIAL_AngleForValue(&g_speedo, 100), s->arc1);
    DIAL_DrawArcSweep(&g_speedo, DIAL_AngleForValue(&g_speedo, 100), DIAL_AngleForValue(&g_speedo, 180), s->arc2);
    DIAL_DrawArcSweep(&g_speedo, DIAL_AngleForValue(&g_speedo, 180), SPEEDO_END, s->arc3);
  }
  DIAL_DrawTicks(&g_speedo, 20, 40, s->tickMinor, s->tickMajor);
  DIAL_DrawCenterHub(&g_speedo, 12, s->hub);
  GFX_DrawStringScaled(g_speedo.cx - 30, g_speedo.cy - 9, "MPH", s->logo, s->bg, 2);
}

void Gauge_Init(void)
{
  const StyleConfig *s = GetStyle();
  DIAL_InitTable();
  LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, s->bg);
  g_currentPage = GAUGE_PAGE_DASHBOARD;
  g_dashStyle = DASH_STYLE_OEM;
  g_prevNeedleVal = 0xFFFFFFFF;
  for (int i = 0; i < 4; i++) g_prevDig[i] = 0xFFFFFFFF;

  DrawTachoFace();
  DrawSpeedoFace();
  DigLabel(0, "Coolant");
  DigLabel(1, "Speed");
  DigLabel(2, "Battery");
  DigLabel(3, "Gear");

  char tmp[8];
  snprintf(tmp, sizeof(tmp), "%d/%d", g_currentPage + 1, GAUGE_PAGE_COUNT);
  GFX_DrawStringCenter(PAGE_FOOTER_Y, tmp, COLOR_GRAY, s->bg);
}

void Gauge_Update(void)
{
  if (OS_GetTimeMs() - g_lastUpdate < 16)
    return;
  g_lastUpdate = OS_GetTimeMs();

  switch (g_currentPage)
  {
    case GAUGE_PAGE_DASHBOARD:
    {
      const StyleConfig *s = GetStyle();
      uint32_t rpm = g_sdsData.rpm;
      if (rpm > TACH_MAX) rpm = TACH_MAX;
      DIAL_DrawNeedleWithWidth(&g_tach, rpm, s->needle, s->bg, g_prevNeedleVal, s->needleWidth);
      g_prevNeedleVal = rpm;

      uint32_t speedMph = (uint32_t)((unsigned long)g_sdsData.speed * 621371UL / 1000000UL);
      if (speedMph > SPEEDO_MAX) speedMph = SPEEDO_MAX;
      DIAL_DrawNeedleWithWidth(&g_speedo, speedMph, s->needle, s->bg, g_prevSpeedVal, s->needleWidth);
      g_prevSpeedVal = speedMph;

      char rpmStr[16];
      snprintf(rpmStr, sizeof(rpmStr), "%u RPM", g_sdsData.rpm);
      if (strcmp(rpmStr, g_prevRpmStr) != 0)
      {
        int16_t rw = strlen(rpmStr) * FONT_STEP * 2;
        LCD_FillRect(TACH_CX - rw/2 - 4, TACH_CY + 35 - 4, TACH_CX + rw/2 + 4, TACH_CY + 35 + 13*2 + 4, s->bg);
        GFX_DrawStringScaled(TACH_CX - rw/2, TACH_CY + 35, rpmStr, s->digit, s->bg, 2);
        strcpy(g_prevRpmStr, rpmStr);
      }

      uint8_t gear = g_sdsData.gearPos;
      if (gear > 6) gear = 0;
      if (gear != g_prevGear)
      {
        g_prevGear = gear;
        int16_t gearX = TACH_CX;
        int16_t gearY = TACH_CY - 50;
        uint16_t gearCol = (gear == 0) ? RGB565(100, 200, 100) : s->gear;
        int16_t tw = strlen(gearStr[gear]) * FONT_STEP * 4;
        int16_t th = FONT_H * 4;
        int16_t pw = tw + 16;
        int16_t ph = th + 8;
        LCD_FillRect(gearX - pw/2 - 2, gearY - ph/2 - 2, gearX + pw/2 + 2, gearY + ph/2 + 2, s->bg);
        LCD_DrawRect(gearX - pw/2, gearY - ph/2, gearX + pw/2, gearY + ph/2, gearCol);
        GFX_DrawStringScaled(gearX - tw/2, gearY - th/2, gearStr[gear], gearCol, s->bg, 4);
      }

      uint32_t dig[4] = { g_sdsData.coolantTemp, speedMph, g_sdsData.batteryVolt, g_sdsData.gearPos };
      const char *units[4] = { "C", "mph", "V", "" };
      for (int i = 0; i < 4; i++)
      {
        if (dig[i] != g_prevDig[i])
        {
          g_prevDig[i] = dig[i];
          DigValueU32(i, dig[i], units[i], s->digit);
        }
      }

      if (g_detailMode)
      {
        if (!g_detailDrawn)
        {
          GFX_DrawStringScaled(15, 165, "MAP", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 185, "IAT", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 205, "TPS", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 225, "O2", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 245, "Inject", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 265, "Ign", s->detailLabel, s->detailBg, 1);
          GFX_DrawStringScaled(15, 285, "IAC", s->detailLabel, s->detailBg, 1);
          g_detailDrawn = 1;
        }

        uint32_t detailVals[8] = { g_sdsData.mapKpa, g_sdsData.intakeAirTemp, g_sdsData.throttlePos, g_sdsData.o2Sensor,
                                   g_sdsData.injectorPulse, g_sdsData.ignitionTiming, g_sdsData.iacStep, 0 };
        const char *detailUnits[8] = { "kPa", "C", "%", "", "ms", "deg", "", "" };
        int16_t detailY[8] = { 165, 185, 205, 225, 245, 265, 285, 0 };
        for (int i = 0; i < 7; i++)
        {
          if (detailVals[i] != g_prevDetailVals[i])
          {
            g_prevDetailVals[i] = detailVals[i];
            char buf[16];
            snprintf(buf, sizeof(buf), "%lu %s", (unsigned long)detailVals[i], detailUnits[i]);
            int16_t w = strlen(buf) * FONT_STEP;
            int16_t h = FONT_H;
            LCD_FillRect(80, detailY[i], 80 + w, detailY[i] + h, s->detailBg);
            GFX_DrawStringScaled(80, detailY[i], buf, s->digit, s->detailBg, 1);
          }
        }
      }
      else
      {
        if (g_detailDrawn)
        {
          LCD_FillRect(10, 160, 290, 310, s->bg);
          g_detailDrawn = 0;
        }
        for (int i = 0; i < 8; i++) g_prevDetailVals[i] = 0xFFFFFFFF;
      }
      break;
    }

    case GAUGE_PAGE_SENSORS:
    {
      if (g_pageDirty)
      {
        LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 21, RGB565(30, 30, 30));
        GFX_DrawStringScaled(30, 80, "MAP", COLOR_GRAY, RGB565(30, 30, 30), 2);
        GFX_DrawStringScaled(30, 80 + DIG_STEP, "O2", COLOR_GRAY, RGB565(30, 30, 30), 2);
        GFX_DrawStringScaled(30, 80 + DIG_STEP * 2, "Ign Timing", COLOR_GRAY, RGB565(30, 30, 30), 2);
        GFX_DrawStringScaled(30, 80 + DIG_STEP * 3, "IAC Steps", COLOR_GRAY, RGB565(30, 30, 30), 2);
        g_pageDirty = 0;
        g_prevSensors[0] = g_sdsData.mapKpa;
        g_prevSensors[1] = g_sdsData.o2Sensor;
        g_prevSensors[2] = g_sdsData.ignitionTiming;
        g_prevSensors[3] = g_sdsData.iacStep;
      }
      else
      {
        uint32_t sensors[4] = { g_sdsData.mapKpa, g_sdsData.o2Sensor, g_sdsData.ignitionTiming, g_sdsData.iacStep };
        const char *units[4] = { "kPa", "", "deg", "" };
        for (int i = 0; i < 4; i++)
        {
          if (sensors[i] != g_prevSensors[i])
          {
            g_prevSensors[i] = sensors[i];
            DigValueU32(i, sensors[i], units[i], COLOR_WHITE);
          }
        }
      }
      break;
    }

    case GAUGE_PAGE_DTC:
    {
      uint8_t dtcBuf[32];
      uint16_t count = SDS_GetDTCs(dtcBuf, sizeof(dtcBuf));

      if (g_pageDirty || count != g_prevDtcCount)
      {
        if (g_pageDirty)
        {
          LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 21, RGB565(30, 30, 30));
        }
        else
        {
          LCD_FillRect(30, 80, 250, 130, RGB565(30, 30, 30));
          for (int i = 0; i < 6; i++)
            LCD_FillRect(30, 130 + i * 30, 250, 130 + (i + 1) * 30, RGB565(30, 30, 30));
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "DTC Count: %u", count);
        GFX_DrawStringScaled(30, 80, buf, COLOR_WHITE, RGB565(30, 30, 30), 2);
        if (count > 0)
        {
          for (uint16_t i = 0; i < count && i < 6; i++)
          {
            snprintf(buf, sizeof(buf), "  %02X", dtcBuf[i]);
            GFX_DrawStringScaled(30, 130 + i * 30, buf, COLOR_RED, RGB565(30, 30, 30), 2);
          }
        }
        g_prevDtcCount = count;
        g_pageDirty = 0;
      }
      break;
    }

    case GAUGE_PAGE_SETTINGS:
    {
      uint8_t settings[4] = {
        KLine_IsConnected() ? 1 : 0,
        BT_IsConnected() ? 1 : 0,
        SD_Log_IsMounted() ? 1 : 0,
        SD_Log_IsActive() ? 1 : 0
      };
      const char *labels[4] = { "K-Line:", "BT:", "SD:", "Log:" };
      const char *values[4][2] = {
        { "NC", "OK" },
        { "NC", "OK" },
        { "NC", "OK" },
        { "OFF", "ON" }
      };
      const uint16_t colors[4][2] = {
        { COLOR_RED, COLOR_GREEN },
        { COLOR_RED, COLOR_GREEN },
        { COLOR_RED, COLOR_GREEN },
        { COLOR_GRAY, COLOR_GREEN }
      };

      if (g_pageDirty)
      {
        LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 21, RGB565(30, 30, 30));
        for (int i = 0; i < 4; i++)
        {
          GFX_DrawStringScaled(30, 80 + i * DIG_STEP, labels[i], COLOR_GRAY, RGB565(30, 30, 30), 2);
          DigValue(i, values[i][settings[i]], colors[i][settings[i]]);
        }
        for (int i = 0; i < 4; i++) g_prevSettings[i] = settings[i];
        g_pageDirty = 0;
      }
      else
      {
        for (int i = 0; i < 4; i++)
        {
          if (settings[i] != g_prevSettings[i])
          {
            g_prevSettings[i] = settings[i];
            DigValue(i, values[i][settings[i]], colors[i][settings[i]]);
          }
        }
      }
      break;
    }

    case GAUGE_PAGE_ABOUT:
    {
      if (g_pageDirty || !g_aboutDrawn)
      {
        LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 21, RGB565(20, 20, 20));
        
        int16_t lineY = 20;
        
        GFX_DrawStringCenterScaled(lineY, "Suzuki ECU Tool", RGB565(0, 180, 255), RGB565(20, 20, 20), 3);
        lineY += 45;
        GFX_DrawStringCenterScaled(lineY, "2004 GSX-R1000", RGB565(180, 180, 180), RGB565(20, 20, 20), 2);
        lineY += 40;
        GFX_DrawStringCenterScaled(lineY, "Version 1.x", RGB565(200, 200, 200), RGB565(20, 20, 20), 2);
        lineY += 35;
        GFX_DrawStringCenterScaled(lineY, __DATE__, RGB565(180, 180, 180), RGB565(20, 20, 20), 2);
        lineY += 35;
        GFX_DrawStringCenterScaled(lineY, "Maker: Uronour", RGB565(200, 200, 200), RGB565(20, 20, 20), 2);
        lineY += 35;
        GFX_DrawStringCenterScaled(lineY, "AI: OpenCode (claude-sonnet)", RGB565(180, 180, 180), RGB565(20, 20, 20), 2);
        
        lineY += 45;
        GFX_DrawStringCenterScaled(lineY, "github.com/uronour/", RGB565(120, 120, 120), RGB565(20, 20, 20), 1);
        lineY += 25;
        GFX_DrawStringCenterScaled(lineY, "suzuki-ecu-tool", RGB565(120, 120, 120), RGB565(20, 20, 20), 1);
        
        lineY += 35;
        GFX_DrawStringCenterScaled(lineY, "GPL-3.0", RGB565(80, 80, 80), RGB565(20, 20, 20), 1);
        g_aboutDrawn = 1;
        g_pageDirty = 0;
      }
      break;
    }
  }

  char pageStr[8];
  snprintf(pageStr, sizeof(pageStr), "%d/%d", g_currentPage + 1, GAUGE_PAGE_COUNT);
  uint16_t footerBg = (g_currentPage == GAUGE_PAGE_DASHBOARD) ? GetStyle()->bg : RGB565(30, 30, 30);
  GFX_DrawStringCenter(PAGE_FOOTER_Y, pageStr, COLOR_GRAY, footerBg);
}

void Gauge_SetPage(GaugePage page)
{
  if (page < GAUGE_PAGE_COUNT)
  {
    g_currentPage = page;
    g_prevNeedleVal = 0xFFFFFFFF;
    for (int i = 0; i < 4; i++) g_prevDig[i] = 0xFFFFFFFF;
    g_detailMode = 0;
    g_detailDrawn = 0;
    for (int i = 0; i < 8; i++) g_prevDetailVals[i] = 0xFFFFFFFF;
    memset(g_prevRpmStr, 0, sizeof(g_prevRpmStr));
    for (int i = 0; i < 4; i++) g_prevSensors[i] = 0xFFFFFFFF;
    g_prevDtcCount = 0xFFFF;
    for (int i = 0; i < 4; i++) g_prevSettings[i] = 0xFF;
    g_aboutDrawn = 0;
    g_pageDirty = 1;

    if (page == GAUGE_PAGE_DASHBOARD)
    {
      const StyleConfig *s = GetStyle();
      LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, s->bg);
      DrawTachoFace();
      DigLabel(0, "Coolant");
      DigLabel(1, "Speed");
      DigLabel(2, "Battery");
      DigLabel(3, "Gear");
    }
    Gauge_Update();
  }
}

GaugePage Gauge_GetPage(void) { return g_currentPage; }

void Gauge_Press(void)
{
  if (g_currentPage == GAUGE_PAGE_DASHBOARD)
  {
    g_detailMode = !g_detailMode;
    if (g_detailMode)
    {
      const StyleConfig *s = GetStyle();
      GFX_DrawStringScaled(15, 165, "MAP", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 185, "IAT", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 205, "TPS", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 225, "O2", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 245, "Inject", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 265, "Ign", s->detailLabel, s->bg, 1);
      GFX_DrawStringScaled(15, 285, "IAC", s->detailLabel, s->bg, 1);
    }
  }
  else if (g_currentPage == GAUGE_PAGE_SETTINGS)
  {
    if (SD_Log_IsActive())
      SD_Log_Stop();
    else
      SD_Log_Start();
  }
}

void Gauge_NextPage(void)
{
  Gauge_SetPage((g_currentPage + 1) % GAUGE_PAGE_COUNT);
}

void Gauge_PrevPage(void)
{
  Gauge_SetPage((g_currentPage == 0) ? (GAUGE_PAGE_COUNT - 1) : (g_currentPage - 1));
}

void Gauge_SetDashStyle(DashStyle style)
{
  if (style < DASH_STYLE_COUNT)
  {
    g_dashStyle = style;
    g_pageDirty = 1;
    Gauge_Update();
  }
}

DashStyle Gauge_GetDashStyle(void)
{
  return g_dashStyle;
}
