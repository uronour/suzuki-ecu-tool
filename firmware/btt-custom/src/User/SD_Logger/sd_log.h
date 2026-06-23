#ifndef _SD_LOG_H_
#define _SD_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

bool SD_Log_Init(void);
bool SD_Log_Start(void);
void SD_Log_Stop(void);
void SD_Log_Tick(void);
bool SD_Log_IsActive(void);
bool SD_Log_IsMounted(void);

#ifdef __cplusplus
}
#endif

#endif
