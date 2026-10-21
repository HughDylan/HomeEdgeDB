#ifndef __TIMEBASE_H
#define __TIMEBASE_H

#include "stm32f10x.h"

void TimeBase_Init(void);
uint32_t TimeBase_GetMs(void);

#endif
