#ifndef _FSMC_H_
#define _FSMC_H_

#include "stdint.h"

#define FSMC_DATA_ADDRESS			((volatile uint8_t*) 0x60010000) 	// D/C = 1
#define FSMC_COMMAND_ADDRESS		((volatile uint8_t*) 0x60000000) 	// D/C = 0

#define fsmc_read(addr)				((*addr))
#define fsmc_write(addr, val)		((*addr) = (val))
void fsmc_init(void);

#endif //_FSMC_H_
