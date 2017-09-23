#ifndef _SGTL5000_H_
#define _SGTL5000_H_

#include "stdint.h"

/*
 * 	Defines
 */
#define SGTL5000_MAX_ADC_GAIN 	 	225 //0.1*dB
#define SGTL5000_MIN_ADC_GAIN 	 	0 //0.1*dB
#define SGTL5000_ADC_GAIN_STEP		15UL	// in 0.1*dB
#define SGTL5000_MAX_HP_OUT_VOLUME		120L	 //0.1*dB
#define SGTL5000_MIN_HP_OUT_VOLUME		-515L	 //0.1*dB
#define SGTL5000_HP_OUT_VOLUME_STEP		5UL		// in 0.1*dB

/*
 * 	Public functions
 */
int32_t sgtl5000_init(void);
int32_t sgtl5000_config_clocks(uint32_t sample_rate);
int32_t sgtl5000_set_audio_routing(uint8_t use_dap);
int32_t sgtl5000_set_hp_out_volume(int16_t value);
int32_t sgtl5000_get_hp_out_volume(int16_t* value);

void sgtl5000_dump_registers(void);

#endif // _SGTL5000_H_
