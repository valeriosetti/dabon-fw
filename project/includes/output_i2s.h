#ifndef _OUTPUT_I2S_
#define _OUTPUT_I2S_

#include "stdint.h"

// Functions' return codes
#define OUTPUT_I2S_SUCCESS						0L
#define OUTPUT_I2S_ERROR						-1L
#define OUTPUT_I2S_PLL_CONFIG_NOT_FOUND			-2L

int output_i2s_init(void);
int output_i2s_ConfigurePLL(uint32_t samplig_freq);

#endif // _OUTPUT_I2S_
