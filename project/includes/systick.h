#ifndef _SYSTICK_H_
#define _SYSTICK_H_

#include "stdint.h"

void systick_initialize(void);
uint32_t systick_get_tick_count(void);

void SysTick_Handler(void);

#endif
