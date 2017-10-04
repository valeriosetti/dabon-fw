#ifndef _SGTL5000_H_
#define _SGTL5000_H_

#include "stdint.h"

/*
 * 	Limiting values
 */
#define SGTL5000_MAX_ADC_GAIN 	 	225 //0.1*dB
#define SGTL5000_MIN_ADC_GAIN 	 	0 //0.1*dB
#define SGTL5000_ADC_GAIN_STEP		15UL	// in 0.1*dB
#define SGTL5000_MAX_HP_OUT_VOLUME		120L	 //0.1*dB
#define SGTL5000_MIN_HP_OUT_VOLUME		-515L	 //0.1*dB
#define SGTL5000_HP_OUT_VOLUME_STEP		5UL		// in 0.1*dB

/*
 * 	Allowed audio routing configurations
 */
#define AUDIO_ROUTING_LINE_IN_to_HP_OUT				0
#define AUDIO_ROUTING_I2S_IN_to_HP_OUT				1
#define AUDIO_ROUTING_I2S_IN_to_DAP_to_HP_OUT		2

/*
 * 	Public functions
 */
int32_t sgtl5000_init(void);
int32_t sgtl5000_config_clocks(uint32_t sample_rate);
int32_t sgtl5000_set_audio_routing(uint8_t use_dap);
int32_t sgtl5000_set_hp_out_volume(int16_t value);
int32_t sgtl5000_get_hp_out_volume(int16_t* value);


int sgtl5000_dump_registers(int argc, char *argv[]);
int set_hp_out_volume(int argc, char *argv[]);

#endif // _SGTL5000_H_
