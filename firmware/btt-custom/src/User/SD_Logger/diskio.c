#include "ff.h"
#include "diskio.h"
#include "sd.h"

static DSTATUS g_diskStatus = STA_NOINIT;

DSTATUS disk_initialize(BYTE pdrv)
{
  if (pdrv != 0) return STA_NOINIT;
  if (SD_Init() == 0)
    g_diskStatus &= ~STA_NOINIT;
  return g_diskStatus;
}

DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != 0) return STA_NOINIT;
  return g_diskStatus;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
  if (pdrv != 0) return RES_PARERR;
  if (g_diskStatus & STA_NOINIT) return RES_NOTRDY;
  if (SD_ReadDisk(buff, sector, count) == 0) return RES_OK;
  return RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
  if (pdrv != 0) return RES_PARERR;
  if (g_diskStatus & STA_NOINIT) return RES_NOTRDY;
  if (SD_WriteDisk((uint8_t *)buff, sector, count) == 0) return RES_OK;
  return RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  (void)cmd; (void)buff;
  if (pdrv != 0) return RES_PARERR;
  return RES_OK;
}
