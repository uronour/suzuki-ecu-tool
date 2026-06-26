#include "main.h"
#include "includes.h"
#include "xpt2046.h"
#include "touch_calib.h"
#include "encoder.h"
#include "boot_screen.h"
#include "LCD_Init.h"

RCC_ClocksTypeDef rccClocks;
CLOCKS mcuClocks;

static uint32_t g_lastKeepAlive = 0;
static uint32_t g_lastTouch = 0;
static uint8_t g_touchDown = 0;
static uint32_t g_touchStartTime = 0;
static uint8_t g_touchHandled = 0;
static int16_t g_encAccum = 0;

static uint32_t g_idleStart = 0;
static uint32_t g_lastPageCycle = 0;
static uint8_t g_autoCycle = 0;


static void Touch_Handle(void)
{
  uint8_t pen = XPT2046_Read_Pen();
  if (pen == 0)
  {
    if (!g_touchDown)
    {
      g_touchDown = 1;
      g_touchStartTime = OS_GetTimeMs();
      g_touchHandled = 0;
      g_idleStart = OS_GetTimeMs();
      g_autoCycle = 0;
      
      // Immediate touch response on press down
      uint16_t tx, ty;
      TP_GetCoordinates(&tx, &ty);
      printf("[TOUCH] Press at (%u,%u)\n", tx, ty);
      Gauge_TouchAt(tx, ty);
      g_touchHandled = 1;
    }
  }
  else
  {
    if (g_touchDown)
    {
      g_touchDown = 0;
      g_touchHandled = 0;
    }
  }
}

static void Encoder_Handle(void)
{
  Encoder_Poll();

  int8_t dir = Encoder_GetDir();
  if (dir != 0)
  {
    g_encAccum += dir;
    if (g_encAccum >= 1)
    {
      g_encAccum = 0;
      g_idleStart = OS_GetTimeMs();
      g_autoCycle = 0;
      Gauge_NextPage();
      printf("[ENC] Next page\n");
    }
    else if (g_encAccum <= -1)
    {
      g_encAccum = 0;
      g_idleStart = OS_GetTimeMs();
      g_autoCycle = 0;
      Gauge_PrevPage();
      printf("[ENC] Prev page\n");
    }
  }

  if (Encoder_GetPress())
  {
    g_idleStart = OS_GetTimeMs();
    g_autoCycle = 0;
    printf("[ENC] Press\n");
    Gauge_Press();
  }
}

int main(void)
{
  SystemClockInit();
  SCB->VTOR = VECT_TAB_FLASH;

  RCC_GetClocksFreq(&rccClocks);
  mcuClocks.rccClocks = rccClocks;
  mcuClocks.PCLK1_Timer_Frequency = (rccClocks.PCLK1_Frequency * 2);
  mcuClocks.PCLK2_Timer_Frequency = (rccClocks.PCLK2_Frequency * 2);

  Delay_init();
  OS_InitTimerMs();
  LCD_Init();
  XPT2046_Init();
  TP_LoadCalibration();
  Encoder_Init();

  Boot_Animation();
  Gauge_Init();
  BT_Init();
  SD_Log_Init();

  g_idleStart = OS_GetTimeMs();
  SDS_Init();
  if (!KLine_IsConnected())
    g_idleStart = OS_GetTimeMs();

  g_lastKeepAlive = OS_GetTimeMs();

  for (;;)
  {
    SDS_PollSensorData();
    Gauge_Update();
    BT_Stream();
    SD_Log_Tick();
    Touch_Handle();
    Encoder_Handle();

    if (KLine_IsConnected())
    {
      g_autoCycle = 0;
    }
    else
    {
      if (OS_GetTimeMs() - g_idleStart > 30000)
      {
        g_autoCycle = 1;
      }
    }

    if (g_autoCycle && (OS_GetTimeMs() - g_lastPageCycle > 10000))
    {
      g_lastPageCycle = OS_GetTimeMs();
      Gauge_NextPage();
    }

    if (OS_GetTimeMs() - g_lastKeepAlive >= SDS_KEEPALIVE_MS)
    {
      SDS_SendKeepAlive();
      g_lastKeepAlive = OS_GetTimeMs();
    }
  }
}