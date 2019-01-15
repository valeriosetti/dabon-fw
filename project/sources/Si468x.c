#include "Si468x.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "timer.h"
#include "debug_printf.h"
#include "utils.h"
#include "clock_configuration.h"
#include "spi.h"
#include "string.h"
#include "eeprom.h"
#include "stdlib.h"
#include "systick.h"

#define debug_msg(format, ...)		debug_printf("[si4684] " format, ##__VA_ARGS__)

// Macros
#define Si468x_assert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR8)
#define Si468x_deassert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS8)
#define Si468x_get_int_status()		READ_BIT(GPIOD->IDR, GPIO_IDR_ID6)

// Typedefs
typedef enum{
	POLLING 	= 0,
	INTERRUPT 	= 1
} Si468x_wait_type;

typedef enum{
	AUTOMATIC	= 0,
	LOW_SIDE	= 1,
	HIGH_SIDE	= 2
} injection_type;

// Private functions for commands
static int Si468x_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size);
static int Si468x_rd_reply(uint32_t extra_data_len);
static int Si468x_powerup(void);
static int Si468x_load_init(void);
static int Si468x_host_load_from_flash(uint8_t* img_data, uint32_t len);
static int Si468x_host_load_from_eeprom(uint8_t img_identifier);
	#define LOAD_BOOTLOADER_IMAGE		0x00
	#define LOAD_FM_IMAGE				0x01
	#define LOAD_DAB_IMAGE				0x02
static int Si468x_boot(void);
static int Si468x_get_sys_state(void);
// Private functions for advanced management
void Si468x_wait_for_cts(Si468x_wait_type type);
void Si468x_wait_for_stcint(Si468x_wait_type type);

// DAB
static int Si468x_start_dab(void);
static int Si468x_dab_get_digital_service_list(void);
static int Si468x_dab_get_freq_list(Si468x_DAB_freq_list *list);
void Si468x_get_part_info(Si468x_info *info);
static int Si468x_dab_tune_freq(uint8_t freq, injection_type injection, uint16_t antcap);
static int Si468x_dab_get_ensamble_info(void);
static int Si468x_dab_start_digital_service(uint32_t service_id, uint32_t component_id);
static int Si468x_dab_digrad_status(uint8_t digrad_ack, uint8_t attune, uint8_t stc_ack, Si468x_DAB_digrad_status *status);
static int Si468x_dab_set_freq_list(void);
static int Si468x_dab_set_property(uint16_t property, uint16_t value);
static int Si468x_dab_get_time(void);

// FM
static int Si468x_start_fm(void);
static int Si468x_fm_tune_freq(uint16_t freq);

// List of commands for DAB mode
#define SI468X_CMD_RD_REPLY								0x00
#define SI468X_CMD_POWER_UP						    	0x01
#define SI468X_CMD_HOST_LOAD					    	0x04
#define SI468X_CMD_FLASH_LOAD					    	0x05
#define SI468X_CMD_LOAD_INIT					    	0x06
#define SI468X_CMD_BOOT							    	0x07
#define SI468X_CMD_GET_PART_INFO						0x08
#define SI468X_CMD_GET_SYS_STATE				    	0x09
#define SI468X_CMD_GET_POWER_UP_ARGS                	0x0A
#define SI468X_CMD_READ_OFFSET                          0x10
#define SI468X_CMD_GET_FUNC_INFO                        0x12
#define SI468X_CMD_SET_PROPERTY                         0x13
#define SI468X_CMD_GET_PROPERTY                         0x14
#define SI468X_CMD_WRITE_STORAGE                        0x15
#define SI468X_CMD_READ_STORAGE                         0x16
#define SI468X_CMD_GET_DIGITAL_SERVICE_LIST             0x80
#define SI468X_CMD_START_DIGITAL_SERVICE                0x81
#define SI468X_CMD_STOP_DIGITAL_SERVICE                 0x82
#define SI468X_CMD_GET_DIGITAL_SERVICE_DATA             0x84
#define SI468X_CMD_DAB_TUNE_FREQ                        0xB0
#define SI468X_CMD_DAB_DIGRAD_STATUS                    0xB2
#define SI468X_CMD_DAB_GET_EVENT_STATUS                 0xB3
#define SI468X_CMD_DAB_GET_ENSEMBLE_INFO                0xB4
#define SI468X_CMD_DAB_GET_SERVICE_LINKING_INFO         0xB7
#define SI468X_CMD_DAB_SET_FREQ_LIST                    0xB8
#define SI468X_CMD_DAB_GET_FREQ_LIST                    0xB9
#define SI468X_CMD_DAB_GET_COMPONENT_INFO               0xBB
#define SI468X_CMD_DAB_GET_TIME                         0xBC
#define SI468X_CMD_DAB_GET_AUDIO_INFO                   0xBD
#define SI468X_CMD_DAB_GET_SUBCHAN_INFO                 0xBE
#define SI468X_CMD_DAB_GET_FREQ_INFO                    0xBF
#define SI468X_CMD_DAB_GET_SERVICE_INFO                 0xC0
#define SI468X_CMD_TEST_GET_RSSI                        0xE5
#define SI468X_CMD_DAB_TEST_GET_BER_INFO                0xE8

// List of command for FM mode
#define SI468X_CMD_FM_TUNE_FREQ							0x30


// List of common properties
#define SI468X_PROP_INT_CTL_ENABLE								0x0000
#define SI468X_PROP_DIGITAL_IO_OUTPUT_SELECT                    0x0200
#define SI468X_PROP_DIGITAL_IO_OUTPUT_SAMPLE_RATE               0x0201
#define SI468X_PROP_DIGITAL_IO_OUTPUT_FORMAT                    0x0202
#define SI468X_PROP_DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_1        0x0203
#define SI468X_PROP_DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_2        0x0204
#define SI468X_PROP_DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_3        0x0205
#define SI468X_PROP_DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_4        0x0206
#define SI468X_PROP_AUDIO_ANALOG_VOLUME                         0x0300
#define SI468X_PROP_AUDIO_MUTE                                  0x0301
#define SI468X_PROP_AUDIO_OUTPUT_CONFIG                         0x0302
#define SI468X_PROP_PIN_CONFIG_ENABLE                           0x0800
#define SI468X_PROP_WAKE_TONE_ENABLE                            0x0900
#define SI468X_PROP_WAKE_TONE_PERIOD                            0x0901
#define SI468X_PROP_WAKE_TONE_FREQ                              0x0902
#define SI468X_PROP_WAKE_TONE_AMPLITUDE                         0x0903

// List of properties for DAB mode
#define SI468X_PROP_DAB_TUNE_FE_VARM                            0x1710
#define SI468X_PROP_DAB_TUNE_FE_VARB                            0x1711
#define SI468X_PROP_DAB_TUNE_FE_CFG                             0x1712
#define SI468X_PROP_DIGITAL_SERVICE_INT_SOURCE                  0x8100
#define SI468X_PROP_DIGITAL_SERVICE_RESTART_DELAY               0x8101
#define SI468X_PROP_DAB_DIGRAD_INTERRUPT_SOURCE                 0xB000
#define SI468X_PROP_DAB_DIGRAD_RSSI_HIGH_THRESHOLD              0xB001
#define SI468X_PROP_DAB_DIGRAD_RSSI_LOW_THRESHOLD               0xB002
#define SI468X_PROP_DAB_VALID_RSSI_TIME                         0xB200
#define SI468X_PROP_DAB_VALID_RSSI_THRESHOLD                    0xB201
#define SI468X_PROP_DAB_VALID_ACQ_TIME                          0xB202
#define SI468X_PROP_DAB_VALID_SYNC_TIME                         0xB203
#define SI468X_PROP_DAB_VALID_DETECT_TIME                       0xB204
#define SI468X_PROP_DAB_EVENT_INTERRUPT_SOURCE                  0xB300
#define SI468X_PROP_DAB_EVENT_MIN_SVRLIST_PERIOD                0xB301
#define SI468X_PROP_DAB_EVENT_MIN_SVRLIST_PERIOD_RECONFIG       0xB302
#define SI468X_PROP_DAB_EVENT_MIN_FREQINFO_PERIOD               0xB303
#define SI468X_PROP_DAB_XPAD_ENABLE                             0xB400
#define SI468X_PROP_DAB_DRC_OPTION                              0xB401
#define SI468X_PROP_DAB_CTRL_DAB_MUTE_ENABLE                    0xB500
#define SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGNAL_LEVEL_THRESHOLD    0xB501
#define SI468X_PROP_DAB_CTRL_DAB_MUTE_WIN_THRESHOLD             0xB502
#define SI468X_PROP_DAB_CTRL_DAB_UNMUTE_WIN_THRESHOLD           0xB503
#define SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGLOSS_THRESHOLD         0xB504
#define SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGLOW_THRESHOLD          0xB505
#define SI468X_PROP_DAB_TEST_BER_CONFIG                         0xE800

// Minimum and maximum values for VARB and VARM
#define SI468X_VARB_MIN						-32767
#define SI468X_VARB_MAX						32768
#define SI468X_VARM_MIN						-32767
#define SI468X_VARM_MAX						32768



// List of properties for FM mode
#define SI468X_PROP_FM_TUNE_FE_CFG								0x1712
#define SI468X_PROP_FM_RDS_CONFIG								0x3C02
#define SI468X_PROP_FM_AUDIO_DE_EMPHASIS						0x3900



// Properties
#define SI468X_XTAL_FREQ						19200000L
#define SI468X_XTAL_CL							10	// in pF - that's taken from the crystal's datasheet!
#define SI468X_XTAL_STARTUP_BIAS_CURRENT		800	// in uA
												// See AN649 at section 9.1.5. It was assumed that:
												// 	- CL=10pF
												//	- startup ESR = 5 x run ESR = 5 x 70ohm = 350ohm (max)

// Global variables
#define IN_OUT_BUFF_SIZE		32
uint8_t data_out[IN_OUT_BUFF_SIZE];
uint8_t data_in[IN_OUT_BUFF_SIZE];
uint8_t Si468x_DAB_active;

// status register bits
#define PUP_STATE_mask				0xC0
#define PUP_STATE_BOOTLOADER		0x80
#define PUP_STATE_APPLICATION		0xC0
#define STATUS0_STCINT				0x01
#define STATUS0_DSRVINT				0x10
#define STATUS0_DACQINT				0x20
#define STATUS0_ERRCMD				0x40
#define STATUS0_CTS					0x80


/********************************************************************************
 * BASIC FUNCTIONS
 ********************************************************************************/
/*
 * Initialize the tuner
 * Note: it assumes that the SPI is already initialized
 */
void Si468x_init()
{
	// Configure GPIOs
	//	PD8 -> RSTB (output, push-pull, low at startup)
	//	PD6 -> INT (input with pull-up)
	RCC_GPIOD_CLK_ENABLE();
	Si468x_assert_reset();
	timer_wait_us(3000);
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE8_Msk, MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE8_Pos);
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_MODER_MODE8_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE8_Pos);
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE6_Msk, MODER_INPUT << GPIO_MODER_MODE6_Pos);
	MODIFY_REG(GPIOD->PUPDR, GPIO_PUPDR_PUPD6_Msk, PUPDR_PULL_UP << GPIO_PUPDR_PUPD6_Pos);
}

/********************************************************************************
 * COMMANDS FUNCTIONS
 * The following commands are explained in Silicon Labs AN 649, rev. 1.9
 ********************************************************************************/
/*
 *
 */
static int Si468x_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size)
{
	spi_set_Si468x_CS();
	timer_wait_us(1);
	if (data_out != NULL)
		spi_write(data_out, data_out_size);
	if (data_in != NULL)
		spi_read(data_in, data_in_size);
	timer_wait_us(1);
	spi_release_Si468x_CS();

	// Uncomment the following block to read the SPI's received data for each command
//	if (data_in != NULL) {
//		int i;
//		for (i=0; i<data_in_size; i++) {
//			debug_msg("  rsp byte %d = 0x%x\n", i, data_in[i]);
//		}
//	}
}

/*
 *
 */
static int Si468x_rd_reply(uint32_t extra_data_len)
{
	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 4+extra_data_len);
}

/*
 *
 */
static int Si468x_powerup()
{
	data_out[0] = SI468X_CMD_POWER_UP;
	data_out[1] = 0x80;	// toggle interrupt when CTS is available
	data_out[2] = 0x17;	// external crystal; TR_SIZE=0x7 (see AN649 at section 9.1)
	data_out[3] = (SI468X_XTAL_STARTUP_BIAS_CURRENT/10);	// see comments in the define
	data_out[4] = ((SI468X_XTAL_FREQ & 0x000000FF)>>0);
	data_out[5] = ((SI468X_XTAL_FREQ & 0x0000FF00)>>8);
	data_out[6] = ((SI468X_XTAL_FREQ & 0x00FF0000)>>16);
	data_out[7] = ((SI468X_XTAL_FREQ & 0xFF000000)>>24);
	data_out[8] = (2*SI468X_XTAL_CL/0.381);	// see AN649 at section 9.3,
	data_out[9] = 0x10;	// fixed
	data_out[10] = 0x00;	// fixed
	data_out[11] = 0x00;	// fixed
	data_out[12] = 0x00;	// fixed
	data_out[13] = ((SI468X_XTAL_STARTUP_BIAS_CURRENT/10)/2);	// see AN649 at section 9.2
	data_out[14] = 0x00;	// fixed
	data_out[15] = 0x00;	// fixed

	Si468x_send_cmd(data_out, 16, NULL, 0);
	Si468x_wait_for_cts(POLLING);

	// data_in[3] contains informations about the current device's state
	if ((data_in[3] & PUP_STATE_mask) != PUP_STATE_BOOTLOADER)
		debug_msg("powerup failure\n");
}

/*
 *
 */
static int Si468x_load_init()
{
	data_out[0] = SI468X_CMD_LOAD_INIT;
	data_out[1] = 0x00;

	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);
}

/*
 * Loads the binary image which is included into the SMT32's fimrware
 */
static int Si468x_host_load_from_flash(uint8_t* img_data, uint32_t len)
{
	uint32_t curr_len;

	// Each command can send up to 4096 bytes. Therefore, if the sent
	// image is bigger, then split it into consecutive chucks
	do {
		data_out[0] = SI468X_CMD_HOST_LOAD;
		data_out[1] = 0x00;
		data_out[2] = 0x00;
		data_out[3] = 0x00;

		curr_len = (len > 4096) ? 4096 : len;

		spi_set_Si468x_CS();
		timer_wait_us(1);
		spi_write(data_out, 4);
		spi_write(img_data, curr_len);
		timer_wait_us(1);
		spi_release_Si468x_CS();

		Si468x_wait_for_cts(POLLING);

		len -= curr_len;
		img_data += curr_len;
	} while(len > 0);
}

/*
 * Load the binary taking it from the eeprom
 */
static int Si468x_host_load_from_eeprom(uint8_t img_identifier)
{
	PARTITION_INFO* eeprom_part_info;
	uint8_t img_buff[EEPROM_PAGE_SIZE_IN_BYTES];

	switch(img_identifier) {
	case LOAD_BOOTLOADER_IMAGE:
		eeprom_part_info = eeprom_get_partition_infos("bootloader");
		break;
	case LOAD_FM_IMAGE:
		eeprom_part_info = eeprom_get_partition_infos("fm_radio");
		break;
	case LOAD_DAB_IMAGE:
		eeprom_part_info = eeprom_get_partition_infos("dab_radio");
		break;
	default:
		debug_msg("error: image not found on the eeprom");
		return -1;
	}

	uint32_t len = eeprom_part_info->data_size;
	uint32_t curr_page = eeprom_part_info->start_page;
	uint32_t curr_len;

	// Read the eeprom page-by-page and send it to the tuner
	do {
		eeprom_page_read(curr_page, img_buff);
		data_out[0] = SI468X_CMD_HOST_LOAD;
		data_out[1] = 0x00;
		data_out[2] = 0x00;
		data_out[3] = 0x00;

		curr_len = (len > sizeof(img_buff)) ? sizeof(img_buff) : len;

		spi_set_Si468x_CS();
		timer_wait_us(1);
		spi_write(data_out, 4);
		spi_write(img_buff, curr_len);
		timer_wait_us(1);
		spi_release_Si468x_CS();

		Si468x_wait_for_cts(POLLING);

		curr_page++;
		len -= curr_len;
	} while(len > 0);
}

/*
 *
 */
static int Si468x_boot()
{
	data_out[0] = SI468X_CMD_BOOT;
	data_out[1] = 0x00;

	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	// data_in[3] contains informations about the current device's state
	if ((data_in[3] & PUP_STATE_mask) != PUP_STATE_APPLICATION)
		debug_msg("boot failure\n");
}

/*
 *
 */
void Si468x_get_part_info(Si468x_info *info)
{
	data_out[0] = SI468X_CMD_GET_PART_INFO;
	data_out[1] = 0;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 22);

	if(data_in[0] & (STATUS0_CTS | STATUS0_ERRCMD))
	{
		/*
		 * AN649Rev1.9 states that the answer to SI468X_CMD_GET_PART_INFO
		 * is 10 byte, four for status [STATUS0, X, X, STATUS3] and six
		 * for the actual answer [CHIPREV, ROMID, X, X, PART[7:0],
		 */

		info->chiprev 	= data_in[4];
		info->romid 	= data_in[5];
		info->part		= ((uint16_t)data_in[9] << 8) | data_in[8];

		debug_msg("Chip rev.: %u\n", info->chiprev);
		debug_msg("ROM ID: %u\n", info->romid);
		debug_msg("Part number: %u\n", info->part);
	}
}

/*
 *
 */
static int Si468x_get_sys_state()
{
	data_out[0] = SI468X_CMD_GET_SYS_STATE;
	data_out[1] = 0x00;

	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_rd_reply(2);

	switch(data_in[4]) {
		case 0:
			debug_msg("Bootloader mode\n");
			break;
		case 1:
			debug_msg("FMHD mode\n");
			break;
		case 2:
			debug_msg("DAB mode\n");
			break;
		default:
			debug_msg("Unknown/don't care\n");
			break;
	}
}



/*
 *
 */
static int Si468x_dab_get_freq_list(Si468x_DAB_freq_list *list)
{
	data_out[0] = SI468X_CMD_DAB_GET_FREQ_LIST;
	data_out[1] = 0x00;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	// Waits for CTS
	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 12);

	if(data_in[0] & (STATUS0_CTS | STATUS0_ERRCMD))
	{
		list->num_freqs = data_in[4];
		list->first_freq = (((uint32_t)data_in[11] << 24) |
							((uint32_t)data_in[10] << 16) |
							((uint32_t)data_in[9] << 8) |
							data_in[8]);

		debug_msg("Number of frequencies: %u\n", list->num_freqs);
		debug_msg("First available frequency: %u kHz\n", list->first_freq);

		return SI468X_SUCCESS;
	} else
		return SI468X_ERROR;
}

/*
 *
 */
static int Si468x_dab_set_freq_list(void)
{
	data_out[0] = SI468X_CMD_DAB_SET_FREQ_LIST;
	data_out[1] = 1;
	data_out[2] = 0;
	data_out[3] = 0;
	data_out[4] = 0x20;
	data_out[5] = 0x78;
	data_out[6] = 0x03;
	data_out[7] = 0x00;
	Si468x_send_cmd(data_out, 8, NULL, 0);

	Si468x_wait_for_cts(POLLING);
}

/*
 *
 */
static int Si468x_dab_tune_freq(uint8_t freq, injection_type injection, uint16_t antcap)
{
	data_out[0] = SI468X_CMD_DAB_TUNE_FREQ;
	data_out[1] = injection;
	data_out[2] = freq;
	data_out[3] = 0x00;
	data_out[4] = antcap & 0x00FF;
	data_out[5] = (antcap & 0xFF00) >> 8;
	Si468x_send_cmd(data_out, 6, NULL, 0);

	Si468x_wait_for_cts(POLLING);

//	Si468x_rd_reply(0);
//	while(!(data_in[0] & (STATUS0_CTS)))
//	{
//		debug_msg("data_in[0] = %02x\n", data_in[0]);
//		systick_wait_for_ms(50);
//		Si468x_rd_reply(0);
//		debug_msg("data_in[0] = %02x\n", data_in[0]);
//	}


	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_digrad_status(uint8_t digrad_ack, uint8_t attune,
									uint8_t stc_ack, Si468x_DAB_digrad_status *status)
{
	data_out[0] = SI468X_CMD_DAB_DIGRAD_STATUS;
	data_out[1] = 0x01 & stc_ack;
	if(digrad_ack)
		data_out[1] |= 0x08;
	if(attune)
		data_out[1] |= 0x04;

	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 23);

	// Interrupts
	status->interrupts = data_in[4];

	// States
	status->states = data_in[5];

	status->rssi 			= (int8_t)data_in[6];
	status->snr 				= data_in[7];
	status->fic_quality 		= data_in[8];
	status->cnr				= data_in[9];
	status->FIB_error_count = data_in[10] | (uint16_t)(data_in[11] << 8);
	status->tune_freq		= 	(uint32_t)data_in[12] |
								(uint32_t)(data_in[13] << 8) |
								(uint32_t)(data_in[14] << 16)|
								(uint32_t)(data_in[15] << 24);
	status->tune_index		= data_in[16];
	status->fft_offset		= (int8_t)data_in[17];
	status->readantcap		= (uint16_t)data_in[18] | (uint16_t)(data_in[19] << 8);
	status->culevel			= (uint16_t)data_in[20] | (uint16_t)(data_in[21] << 8);
	status->fastdect			= data_in[22];

	debug_msg("DAB status RSSI: %d \n", Si468x_DAB_status.rssi);
	debug_msg("DAB status SNR: %d \n", Si468x_DAB_status.snr);
	debug_msg("DAB status FIC quality: %d \n", Si468x_DAB_status.fic_quality);
	debug_msg("DAB status tune frequency: %d \n", Si468x_DAB_status.tune_freq);
	debug_msg("DAB status antenna cap: %d \n", Si468x_DAB_status.readantcap);
	debug_msg("DAB status CNR: %d \n", Si468x_DAB_status.cnr);
	debug_msg("DAB status, Flags: Valid %d - Acq: %d - FIC error: %d \n",
		 (Si468x_DAB_status.states & 0x01) ? 1 : 0,
		 (Si468x_DAB_status.states & 0x04) ? 1 : 0,
		 (Si468x_DAB_status.states & 0x08) ? 1 : 0);

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_get_ensamble_info(void)
{
	uint8_t i;

	data_out[0] = SI468X_CMD_DAB_GET_ENSEMBLE_INFO;
	data_out[1] = 0x00;

	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 26);

	Si468x_DAB_ensamble_infos.eid = (uint16_t)((data_in[5] << 8) | data_in[4]);
	for(i = 0; i < 16; i++)
	{
		Si468x_DAB_ensamble_infos.label[i] = data_in[6 + i];
	}
	Si468x_DAB_ensamble_infos.label[16] = '\0';
	Si468x_DAB_ensamble_infos.ensamble_ecc = data_in[22];
	Si468x_DAB_ensamble_infos.char_abbrev = (uint16_t)((data_in[25] << 8) | data_in[24]);

    debug_msg("Ensamble info, EID: %u\n", Si468x_DAB_ensamble_infos.eid);
	debug_msg("Ensamble info, Label: %s\n", Si468x_DAB_ensamble_infos.label);
	debug_msg("Ensamble info, EID: %u\n", Si468x_DAB_ensamble_infos.ensamble_ecc);
	debug_msg("Ensamble info, EID: %02X\n", Si468x_DAB_ensamble_infos.char_abbrev);
    
	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_get_time(void)
{
	data_out[0] = SI468X_CMD_DAB_GET_TIME;
	data_out[1] = 0x00;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 11);

	Si468x_DAB_current_time.year = ((uint16_t)(data_in[5] << 8) | data_in[4]);
	Si468x_DAB_current_time.month = data_in[6];
	Si468x_DAB_current_time.days = data_in[7];
	Si468x_DAB_current_time.hours = data_in[8];
	Si468x_DAB_current_time.minutes = data_in[9];
	Si468x_DAB_current_time.seconds = data_in[10];

	debug_msg("Current time: %d/%d/%d %d:%d:%d\n",
			Si468x_DAB_current_time.year,
			Si468x_DAB_current_time.month,
			Si468x_DAB_current_time.days,
			Si468x_DAB_current_time.hours,
			Si468x_DAB_current_time.minutes,
			Si468x_DAB_current_time.seconds);

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_get_digital_service_list(void)
{
	uint16_t service_list_size = 0;
	uint16_t service_list_version = 0;
	uint8_t num_of_services = 0;
	uint8_t digital_service_list_buffer[2048];

	// Fills data_out buffer with "Get digital service list" and the command argument
	data_out[0] = SI468X_CMD_GET_DIGITAL_SERVICE_LIST;
	data_out[1] = 0x01;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	// Waits for CTS
	Si468x_wait_for_cts(POLLING);

	// Parses reply from Si4684
	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, digital_service_list_buffer, 2048);

	service_list_size = (uint16_t)((digital_service_list_buffer[5] << 8) |
										digital_service_list_buffer[4]);
	service_list_version = (uint16_t)((digital_service_list_buffer[7] << 8) |
										digital_service_list_buffer[6]);
	num_of_services = digital_service_list_buffer[8];

    debug_msg("Service list size (bytes): %u\n", service_list_size);
	debug_msg("Service list version: %u\n", service_list_version);
	debug_msg("Number of services: %u\n", num_of_services);
    
    uint8_t* buff_ptr = &digital_service_list_buffer[12];
    uint8_t service_index;
    
    for (service_index = 0; service_index < num_of_services; service_index++) {
        Si468x_DAB_digital_service* service = (Si468x_DAB_digital_service*)buff_ptr;
        debug_msg("Service ID: 0x%x(%u) | info 1: 0x%x | info 2: 0x%x | info 3: 0x%x | label: %.15s\n", 
                service->service_id, service->service_id, service->service_info_1, service->service_info_2, service->service_info_3, service->service_label);
        buff_ptr += sizeof(Si468x_DAB_digital_service);

        uint8_t number_of_components = service->service_info_2 & 0x0F;
        uint8_t component_index;
        for (component_index = 0; component_index < number_of_components; component_index++) {
            Si468x_DAB_digital_service_component* component = (Si468x_DAB_digital_service_component*)buff_ptr;
            debug_msg("   Component ID: %u(%u)\n", component->component_id, component->component_id);
            buff_ptr += sizeof(Si468x_DAB_digital_service_component);
        }
    }

	return SI468X_SUCCESS;
}


/*
 *
 */
static int Si468x_dab_start_digital_service(uint32_t service_id, uint32_t component_id)
{
    debug_msg("Starting service id: 0x%x - component id: 0x%x\n", service_id, component_id);
    
	data_out[0] = SI468X_CMD_START_DIGITAL_SERVICE;
	data_out[1] = 0x00;
	data_out[2] = 0x00;
	data_out[3] = 0x00;
	data_out[4] = (uint8_t)(service_id & 0x000000FF);
	data_out[5] = (uint8_t)((service_id & 0x0000FF00) >> 8);
	data_out[6] = (uint8_t)((service_id & 0x00FF0000) >> 16);
	data_out[7] = (uint8_t)((service_id & 0xFF000000) >> 24);
	data_out[8] = (uint8_t)(component_id & 0x000000FF);
	data_out[9] = (uint8_t)((component_id & 0x0000FF00) >> 8);
	data_out[10] = (uint8_t)((component_id & 0x00FF0000) >> 16);
	data_out[11] = (uint8_t)((component_id & 0xFF000000) >> 24);
	Si468x_send_cmd(data_out, 12, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_get_event_status(void)
{
	data_out[0] = SI468X_CMD_DAB_GET_EVENT_STATUS;
	data_out[1] = 0x01;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 8);

	// TODO
	// complete the events list, now has only partial informations!
	debug_msg("Service list interrupt: %d\n", (data_in[5] & 0x01) ? 0x01 : 0x00);


	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_dab_get_audio_info()
{
    data_out[0] = SI468X_CMD_DAB_GET_AUDIO_INFO;
    data_out[1] = 0x01;
	Si468x_send_cmd(data_out, 2, NULL, 0);
    
    Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 10);
    
    uint16_t bit_rate = ((uint16_t)(data_in[5] << 8) | (uint16_t)(data_in[4]));
    uint16_t sample_rate = ((uint16_t)(data_in[7] << 8) | (uint16_t)(data_in[6]));
    uint8_t drc_gain = data_in[9];
    
    debug_msg("Bit rate: %u\n", bit_rate);
    debug_msg("Sample rate: %u\n", sample_rate);
    debug_msg("Audio DRC gain: %u\n", drc_gain);
    switch (data_in[8] & 0x03) {
        case 0:
            debug_msg("Audio mode: dual\n");
            break;
        case 1:
            debug_msg("Audio mode: mono\n");
            break;
        case 2:
            debug_msg("Audio mode: stereo\n");
            break;
        case 3:
            debug_msg("Audio mode: join stereo\n");
            break;
    }
}

/*
 *
 */
static int Si468x_dab_set_property(uint16_t property, uint16_t value)
{
	data_out[0] = SI468X_CMD_SET_PROPERTY;
	data_out[1] = 0x00;
	data_out[2] = (uint8_t)(0x00FF & property);
	data_out[3] = (uint8_t)((0xFF00 & property) >> 8);
	data_out[4] = (uint8_t)(0x00FF & value);
	data_out[5] = (uint8_t)((0xFF00 & value) >> 8);
	Si468x_send_cmd(data_out, 6, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	return SI468X_SUCCESS;
}

/*
 *
 */
static uint16_t Si468x_dab_get_property(uint16_t property)
{
	uint16_t read_value;

	data_out[0] = SI468X_CMD_GET_PROPERTY;
	data_out[1] = 0x01;
	data_out[2] = (uint8_t)(0x00FF & property);
	data_out[3] = (uint8_t)((0xFF00 & property) >> 8);
	Si468x_send_cmd(data_out, 4, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, data_in, 6);

	read_value = ((uint16_t)data_in[4] | ((uint16_t)(data_in[5]) << 8));

	debug_msg("Property %04x : %04x \n", property, read_value);

	return read_value;

}

/********************************************************************************
 * ADVANCED FUNCTIONS
 ********************************************************************************/
/*
 *
 */
void Si468x_wait_for_cts(Si468x_wait_type type)
{
	if(type == POLLING)
	{
		do {
			Si468x_rd_reply(0);
		} while (!(data_in[0] & (STATUS0_CTS)));


	}
	else if(type == INTERRUPT)
	{
		while(!Si468x_get_int_status());
		Si468x_rd_reply(0);
	}
}

/*
 *
 */
void Si468x_wait_for_stcint(Si468x_wait_type type)
{
	if(type == POLLING)
	{
		Si468x_rd_reply(0);
		while(!(data_in[0] & STATUS0_STCINT))
		{
			Si468x_rd_reply(0);
		}
			
	}
	else if(type == INTERRUPT)
	{
		while(Si468x_get_int_status());
	}
}

/*
 *
 */
static int Si468x_start_dab()
{
	uint8_t actual_freq;
	int32_t varb, varm;
	uint8_t varcap_index;
	volatile uint32_t size = 0;
	volatile uint32_t initial = 0, final = 0;

	// Take the tuner out of reset and wait for 3ms
	Si468x_deassert_reset();
	timer_wait_us(3000);
	// Send power-up and then wait 20us
	Si468x_powerup();
	timer_wait_us(20);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the bootloader image
	if (utils_is_fw_embedded(SI468X_BOOT_FW)) {
		Si468x_host_load_from_eeprom(LOAD_BOOTLOADER_IMAGE);
	} else {
		Si468x_host_load_from_flash(utils_get_embedded_FW_start_address(SI468X_BOOT_FW), utils_get_embedded_FW_size(SI468X_BOOT_FW));
	}
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the application image (DAB)
	if (utils_is_fw_embedded(SI468X_DAB_FW)) {
		Si468x_host_load_from_flash(utils_get_embedded_FW_start_address(SI468X_DAB_FW), utils_get_embedded_FW_size(SI468X_DAB_FW));
	} else {
		Si468x_host_load_from_eeprom(LOAD_DAB_IMAGE);
	}
	// Wait for 4ms
	timer_wait_us(4000);
	// Boot the image
	Si468x_boot();

	timer_wait_us(400000);

	Si468x_get_part_info(&Si468x_info_part);

	Si468x_dab_set_property(SI468X_PROP_INT_CTL_ENABLE, 0x0081);
	//Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGNAL_LEVEL_THRESHOLD, 0x0000);
	//Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGLOW_THRESHOLD, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_ENABLE, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_DAB_DIGRAD_INTERRUPT_SOURCE, 0x0001);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_CFG, 0x0001);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARM, 0xF8A9);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARB, 0x01C6);
	Si468x_dab_set_property(SI468X_PROP_PIN_CONFIG_ENABLE, 0x0001);
	//Si468x_dab_set_property(SI468X_PROP_DAB_VALID_DETECT_TIME, 2000);
	//Si468x_dab_set_property(SI468X_PROP_DAB_VALID_ACQ_TIME, 2000);
	//Si468x_dab_set_property(SI468X_PROP_DAB_VALID_SYNC_TIME, 2000);

	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_VARM);
	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_VARB);
	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_CFG);

	Si468x_dab_get_freq_list(&Si468x_freq_list);

//	 for(actual_freq = 0; actual_freq < Si468x_freq_list.num_freqs; actual_freq++)
//	 {
//	 	debug_msg("\nSetting tune frequency to: %u\n", actual_freq);
//
//	 	Si468x_dab_tune_freq(actual_freq, AUTOMATIC, 0);
//
//	 	Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
//	 }

	// debug_msg("Setting tune frequency to: %u\n", 32);
	// Si468x_dab_tune_freq(32, AUTOMATIC, 0);
	// Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	//
	// debug_msg("Setting tune frequency to: %u\n", 33);
	// Si468x_dab_tune_freq(33, AUTOMATIC, 0);
	// Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	//
	// debug_msg("Setting tune frequency to: %u\n", 34);
	// Si468x_dab_tune_freq(34, AUTOMATIC, 0);
	// Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);

	// Si468x_dab_tune_freq(34, AUTOMATIC, 0);
	// Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	// debug_msg("DAB status tune frequency: %d \n", Si468x_DAB_status.tune_freq);
	//
//	debug_msg("VARB, VARM, RSSI, SNR, VALID_FLAG\n");
//
//	for (varb = SI468X_VARB_MIN; varb < SI468X_VARB_MAX; varb = varb + 200)
//	{
//		for(varm = SI468X_VARM_MIN; varm < SI468X_VARM_MAX; varm = varm + 200)
//		{
//			Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARM, varm);
//			Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARB, varb);
//			Si468x_dab_tune_freq(34, AUTOMATIC, 0);
//			Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
//			debug_msg("%d, %d, %d, %d, %d, %d\n", varb, varm,
//										Si468x_DAB_status.rssi,
//										Si468x_DAB_status.snr,
//										Si468x_DAB_status.states.valid,
//										Si468x_DAB_status.states.acq);
//		}
//	}

//	for(varcap_index = 1; varcap_index <= 128; varcap_index++)
//	{
//		 Si468x_dab_tune_freq(34, varcap_index, 0);
//		 Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
//		 debug_msg("%d, %d, %d, %d, %d\n", 	varcap_index,
//		 									Si468x_DAB_status.rssi,
//											Si468x_DAB_status.snr,
//											Si468x_DAB_status.states.valid,
//											Si468x_DAB_status.states.acq);
//	}


	//Si468x_get_digital_service_list();

	Si468x_DAB_active = 1;

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_start_fm()
{
	// Take the tuner out of reset and wait for 3ms
	Si468x_deassert_reset();
	timer_wait_us(3000);
	// Send power-up and then wait 20us
	Si468x_powerup();
	timer_wait_us(20);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the bootloader image
	if (utils_is_fw_embedded(SI468X_BOOT_FW)) {
		Si468x_host_load_from_flash(utils_get_embedded_FW_start_address(SI468X_BOOT_FW), utils_get_embedded_FW_size(SI468X_BOOT_FW));
	} else {
		Si468x_host_load_from_eeprom(LOAD_BOOTLOADER_IMAGE);
	}
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the application image (FM)
	if (utils_is_fw_embedded(SI468X_FM_FW)) {
		Si468x_host_load_from_flash(utils_get_embedded_FW_start_address(SI468X_FM_FW), utils_get_embedded_FW_size(SI468X_FM_FW));
	} else {
		Si468x_host_load_from_eeprom(LOAD_FM_IMAGE);
	}
	// Wait for 4ms
	timer_wait_us(4000);
	// Boot the image
	Si468x_boot();

	timer_wait_us(400000);

	Si468x_get_part_info(&Si468x_info_part);

	Si468x_dab_set_property(SI468X_PROP_PIN_CONFIG_ENABLE, 0x0001);	// Analog audio output
	Si468x_dab_set_property(SI468X_PROP_FM_TUNE_FE_CFG, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_FM_RDS_CONFIG, 0x0001);
	Si468x_dab_set_property(SI468X_PROP_FM_AUDIO_DE_EMPHASIS, 0x0001);

	Si468x_fm_tune_freq(10420);

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_fm_tune_freq(uint16_t freq)
{
	data_out[0] = SI468X_CMD_FM_TUNE_FREQ;
	data_out[1] = 0x00;
	data_out[2] = (uint8_t)(freq & 0xFF);
	data_out[3] = (uint8_t)((freq & 0xFF00) >> 8);
	data_out[4] = 0;
	data_out[5] = 0;
	data_out[6] = 0;
	Si468x_send_cmd(data_out, 7, NULL, 0);

	Si468x_wait_for_cts(POLLING);

	Si468x_wait_for_stcint(POLLING);

	return SI468X_SUCCESS;
}

/********************************************************************************
 * SHELL COMMANDS
 ********************************************************************************/
/*
 * Change current tuner's frequency (FM radio)
 */
int fm_tune(int argc, char *argv[])
{
	if (argc != 1) {
		debug_msg("wrong number of parameters\n");
        return -1;
	}
	Si468x_fm_tune_freq(atoi(argv[0]));
}

/*
 * Start the tuner either in FM or DAB mode
 */
int start_tuner(int argc, char *argv[])
{
    if (argc != 1) {
		debug_msg("wrong number of parameters\n");
        return -1;
	}   
    
    // Reset the tuner and wait for 50ms before reloading the new image
    Si468x_assert_reset();
    systick_wait_for_ms(50);
    
    if (strcmp(argv[0], "fm") == 0) {
        Si468x_start_fm();
        debug_msg("FM firmware loaded\n");
    } else if (strcmp(argv[0], "dab") == 0) {
        Si468x_start_dab();
        debug_msg("DAB firmware loaded\n");
    } else {
        debug_msg("wrong firmware image selected\n");
        return -1;
    }
    return 0;
}

/*
 * Tune DAB to select frequency with specific VARB, VARM and switch
 * position parameters
 */
//int dab_tune_frequency(int argc, char *argv[])
//{
//	int16_t varb, varm;
//
//	if(argc != 4)
//	{
//		debug_msg("please specify frequency, VARB, VARM and switch position\n");
//		return -1;
//	}
//	if(Si468x_DAB_active == 1)
//	{
//		varb = atoi(argv[1]);
//		varm = atoi(argv[2]);
//
//		Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_CFG, (0x0001 & atoi(argv[3])));
//		Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARM, varm);
//		Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARB, varb);
//		Si468x_dab_tune_freq(atoi(argv[0]), AUTOMATIC, 0);
//		Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
//		debug_msg("%d, %d, %d, %d, %u\n",
//										varb,
//										varm,
//										Si468x_DAB_status.rssi,
//										Si468x_DAB_status.snr,
//										Si468x_DAB_status.states.valid);
//	} else
//	{
//		debug_msg("DAB tuner must be active to complete this request!\n");
//		return -1;
//	}
//}
int dab_tune_frequency(int argc, char *argv[])
{
    if (argc != 1) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
	Si468x_dab_tune_freq(atoi(argv[0]), AUTOMATIC, 0);

//	Si468x_DAB_status.states = 0x00;
//	while(!(Si468x_DAB_status.states & 0x04))
//		Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	return SI468X_SUCCESS;
}

int dab_digrad_status(int argc, char *argv[])
{
    if (argc != 0) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
	Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	return SI468X_SUCCESS;
}

int dab_get_event_status(int argc, char *argv[])
{
    if (argc != 0) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
    Si468x_dab_get_event_status();
	return SI468X_SUCCESS;
}

int dab_get_ensamble_info(int argc, char *argv[])
{
    if (argc != 0) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
    Si468x_dab_get_ensamble_info();
	return SI468X_SUCCESS;
}

int dab_get_digital_service_list(int argc, char *argv[])
{
    if (argc != 0) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
    Si468x_dab_get_digital_service_list();
	return SI468X_SUCCESS;
}

int dab_start_digital_service(int argc, char *argv[])
{
    if (argc != 2) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
    Si468x_dab_start_digital_service(atoi(argv[0]), atoi(argv[1]));
	return SI468X_SUCCESS;
}

int dab_get_audio_info(int argc, char *argv[])
{
    if (argc != 0) {
        debug_msg("wrong number of parameters\n");
        return -1;
    }
    Si468x_dab_get_audio_info();
	return SI468X_SUCCESS;
}
