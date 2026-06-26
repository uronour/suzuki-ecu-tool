#include "sd_log.h"
#include "ff.h"
#include "SDSProtocol.h"
#include "kline_task.h"
#include "os_timer.h"
#include <stdio.h>

static FATFS g_fs;
static FIL g_file;
static uint8_t g_mounted = 0;
static uint8_t g_logging = 0;
static uint32_t g_lastLogMs = 0;

bool SD_Log_Init(void)
{
  if (f_mount(&g_fs, "0:", 1) == FR_OK)
  {
    g_mounted = 1;
    return true;
  }
  g_mounted = 0;
  return false;
}

bool SD_Log_Start(void)
{
  if (!g_mounted) return false;
  if (f_open(&g_file, "0:/sds_data.csv", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    return false;
  f_printf(&g_file, "Time(ms),RPM,Speed(kmh),Coolant(C),IAT(C),TPS(%%),MAP(kPa),"
                    "Battery(V),Gear,O2,Injector(ms),Ign(deg),IAC,Clutch,Fan,Sidestand\n");
  g_logging = 1;
  g_lastLogMs = OS_GetTimeMs();
  return true;
}

void SD_Log_Stop(void)
{
  if (g_logging)
  {
    f_close(&g_file);
    g_logging = 0;
  }
}

void SD_Log_Tick(void)
{
  if (!g_logging) return;
  if (OS_GetTimeMs() - g_lastLogMs < 1000) return;
  g_lastLogMs = OS_GetTimeMs();

  f_printf(&g_file, "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
    (unsigned long)OS_GetTimeMs(),
    g_sdsData.rpm,
    g_sdsData.speed,
    g_sdsData.coolantTemp,
    g_sdsData.intakeAirTemp,
    g_sdsData.throttlePos,
    g_sdsData.mapKpa,
    g_sdsData.batteryVolt,
    g_sdsData.gearPos,
    g_sdsData.o2Sensor,
    g_sdsData.injectorPulse,
    g_sdsData.ignitionTiming,
    g_sdsData.iacStep,
    g_sdsData.clutchIn ? 1 : 0,
    g_sdsData.fanOn ? 1 : 0,
    g_sdsData.sidestandDown ? 1 : 0
  );

  f_sync(&g_file);
}

bool SD_Log_IsActive(void)
{
  return g_logging;
}

bool SD_Log_IsMounted(void)
{
  return g_mounted;
}

bool SD_Log_SaveBin(const char *filename, const uint8_t *data, uint32_t len)
{
  if (!g_mounted || !data || len == 0) return false;
  FIL file;
  if (f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    return false;
  unsigned bytesWritten;
  FRESULT res = f_write(&file, data, len, &bytesWritten);
  f_close(&file);
  return (res == FR_OK && bytesWritten == len);
}
