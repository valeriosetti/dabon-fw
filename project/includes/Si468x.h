#ifndef _SI468X_H_
#define _SI468X_H_

#include <stdint.h>

// Functions' return codes
#define SI468X_SUCCESS		0L
#define SI468X_ERROR		-1L

// Public functions
void Si468x_init(void);

// Shell commands
int start_tuner(int argc, char *argv[]);
int fm_tune(int argc, char *argv[]);
int dab_tune_frequency(int argc, char *argv[]);
int dab_digrad_status(int argc, char *argv[]);
int dab_get_event_status(int argc, char *argv[]);
int dab_get_ensamble_info(int argc, char *argv[]);
int dab_get_digital_service_list(int argc, char *argv[]);
int dab_start_digital_service(int argc, char *argv[]);
int dab_get_audio_info(int argc, char *argv[]);

// Typedefs
typedef struct {
	uint8_t chiprev;
	uint8_t romid;
	uint16_t part;
} Si468x_info;

typedef struct {
	uint8_t num_freqs;		// Number of frequencies in the list
	uint32_t first_freq;	// First entry of the list
} Si468x_DAB_freq_list;

typedef struct {
	uint8_t interrupts;
	uint8_t states;
	int8_t rssi;
	uint8_t snr;
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

typedef struct {
	uint16_t eid;
	char label[17];
	uint8_t ensamble_ecc;
	uint16_t char_abbrev;
} Si468x_DAB_ensamble_info;

typedef struct {
	uint32_t service_id;
	uint8_t service_info_1;
	uint8_t service_info_2;
	uint8_t service_info_3;
	uint8_t rsvd;
	char service_label[16];
} Si468x_DAB_digital_service;

typedef struct {
    uint16_t component_id;
    uint8_t component_info;
    uint8_t valid_flags;
} Si468x_DAB_digital_service_component;

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t days;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} Si468x_DAB_time;

// Public variables
Si468x_info Si468x_info_part;
Si468x_DAB_freq_list Si468x_freq_list;
Si468x_DAB_digrad_status Si468x_DAB_status;
Si468x_DAB_ensamble_info Si468x_DAB_ensamble_infos;
Si468x_DAB_time Si468x_DAB_current_time;

#endif //_SI468X_H_
