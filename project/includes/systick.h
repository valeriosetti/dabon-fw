#ifndef _SYSTICK_H_
#define _SYSTICK_H_

#include "stdint.h"

void systick_initialize(void);
uint32_t systick_get_tick_count(void);
void systick_wait_for_ms(uint32_t delay);

void SysTick_Handler(void);

// Shell commands
int systick_gettime(int argc, char *argv[]);

#endif
