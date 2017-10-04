#ifndef _SI468X_H_
#define _SI468X_H_

#include <stdint.h>

// Functions' return codes
#define SI468X_SUCCESS		0L
#define SI468X_ERROR		-1L

#define sizeof_binary_image(_img_name_)		(uint32_t)((&_binary___external_firmwares_##_img_name_##_end)-(&_binary___external_firmwares_##_img_name_##_start))

// Public functions
void Si468x_init(void);

// Shell commands
int fm_tune(int argc, char *argv[]);

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

typedef struct Si468x_DAB_digrad_status{
	struct {
		unsigned rssilint:1;
		unsigned rssihint:1;
		unsigned acqint:1;
		unsigned ficerrint:1;
		unsigned hardmutedint:1;
	} interrupts;
	struct {
		unsigned valid:1;
		unsigned acq:1;
		unsigned ficerr:1;
		unsigned hardmute:1;
	} states;
	int8_t rssi;
	int8_t snr;
	uint8_t fic_quality;
	uint8_t cnr;
	uint16_t FIB_error_count;
	uint32_t tune_freq;
	uint8_t tune_index;
	uint8_t tune_offet;
	int8_t fft_offset;
	uint16_t readantcap;
	uint16_t culevel;
	uint8_t fastdect;
} Si468x_DAB_digrad_status;

// Public variables
Si468x_info Si468x_info_part;
Si468x_DAB_freq_list Si468x_freq_list;
Si468x_DAB_digrad_status Si468x_DAB_status;

#endif //_SI468X_H_
