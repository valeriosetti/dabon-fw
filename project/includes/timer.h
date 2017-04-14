#ifndef _TIMER_H_
#define _TIMER_H_

#include "stdint.h"

void timer_init(void);
void timer_wait_us(uint32_t);

#endif // _TIMER_H_
