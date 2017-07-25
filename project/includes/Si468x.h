#ifndef _SI468X_H_
#define _SI468X_H_

#include <stdint.h>

// Functions' return codes
#define SI468X_SUCCESS		0L
#define SI468X_ERROR		-1L

// Public functions
void Si468x_init(void);

// Typedefs
typedef struct Si468x_info{
	uint8_t chiprev;
	uint8_t romid;
	uint16_t part;
} Si468x_info;

typedef struct Si468x_DAB_freq_list{
	uint8_t num_freqs;		// Number of frequencies in the list
	uint32_t first_freq;	// First entry of the list
} Si468x_DAB_freq_list;

// Public variables
Si468x_info Si468x_info_part;
Si468x_DAB_freq_list Si468x_freq_list;

#endif //_SI468X_H_
