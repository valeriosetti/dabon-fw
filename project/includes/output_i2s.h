#ifndef _OUTPUT_I2S_
#define _OUTPUT_I2S_

#include "stdint.h"

int output_i2s_init(void);
int output_i2s_ConfigurePLL(uint32_t samplig_freq);

void DMA1_Stream7_IRQHandler(void);

#endif // _OUTPUT_I2S_
