#ifndef _OUTPUT_I2S_
#define _OUTPUT_I2S_

#include "stdint.h"

int32_t output_i2s_init(void);
int32_t output_i2s_ConfigurePLL(uint32_t samplig_freq);
int32_t output_i2s_enqueue_samples(int16_t* data, uint16_t samples_count);

void DMA1_Stream7_IRQHandler(void);

#endif // _OUTPUT_I2S_
