#include "Si468x.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "timer.h"
#include "debug_printf.h"
#include "utils.h"
#include "clock_configuration.h"
#include "spi.h"
#include "string.h"

#define debug_msg(...)		debug_printf_with_tag("[Si468x] ", __VA_ARGS__)

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
static int Si468x_host_load(uint8_t* img_data, uint32_t len);
static int Si468x_boot(void);
static int Si468x_get_sys_state(void);
// Private functions for advanced management
void Si468x_wait_for_cts(Si468x_wait_type type);
void Si468x_wait_for_stcint(Si468x_wait_type type);

// DAB
static int Si468x_start_dab(void);
static int Si468x_get_digital_service_list(void);
static int Si468x_dab_get_freq_list(Si468x_DAB_freq_list *list);
void Si468x_get_part_info(Si468x_info *info);
static int Si468x_dab_tune_freq(uint8_t freq, injection_type injection, uint16_t antcap);
static int Si468x_dab_digrad_status(uint8_t digrad_ack, uint8_t attune,
									uint8_t stc_ack, Si468x_DAB_digrad_status *status);
static int Si468x_dab_set_freq_list(void);
static int Si468x_dab_set_property(uint16_t property, uint16_t value);

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
#define SI468X_PROP_INT_CTL_REPEAT								0x0001
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

/********************************************************************************
 * COMMANDS FUNCTIONS
 * The following commands are explained in Silicon Labs AN 649, rev. 1.9
 ********************************************************************************/
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
 *
 */
static int Si468x_host_load(uint8_t* img_data, uint32_t len)
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
static int Si468x_get_digital_service_list(void)
{
	uint32_t i;

	uint8_t digital_service_buffer[2048];

	// Fills data_out buffer with "Get digital service list" and the command argument
	data_out[0] = SI468X_CMD_GET_DIGITAL_SERVICE_LIST;
	data_out[1] = 0x00;
	Si468x_send_cmd(data_out, 2, NULL, 0);

	// Waits for CTS
	Si468x_wait_for_cts(POLLING);

	// Parses reply from Si4684
	data_out[0] = SI468X_CMD_RD_REPLY;
	Si468x_send_cmd(data_out, 1, digital_service_buffer, 2048);

//	for(i=0; i < 2048; i++)
//	{
//		debug_printf("%s", digital_service_buffer[i]);
//	}

	return SI468X_SUCCESS;
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

	Si468x_wait_for_stcint(POLLING);

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
	status->interrupts.rssilint 	= (0x01 & data_in[4]) != 0;
	status->interrupts.rssihint 	= (0x02 & data_in[4]) != 0;
	status->interrupts.acqint 		= (0x04 & data_in[4]) != 0;
	status->interrupts.ficerrint 	= (0x10 & data_in[4]) != 0;
	status->interrupts.hardmutedint = (0x20 & data_in[4]) != 0;

	// States
	status->states.valid 			= (0x01 & data_in[5]) != 0;
	status->states.acq 				= (0x04 & data_in[5]) != 0;
	status->states.ficerr 			= (0x10 & data_in[5]) != 0;

	status->rssi 			= (int8_t)data_in[6];
	status->snr 			= (int8_t)data_in[7];
	status->fic_quality 	= data_in[8];
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
	status->fastdect		= data_in[22];

	debug_msg("DAB status RSSI: %d \n", status->rssi);
	debug_msg("DAB status SNR: %d \n", status->snr);
	debug_msg("DAB status FIC quality: %d \n", status->fic_quality);
	debug_msg("DAB status tune frequency: %d \n", status->tune_freq);
	debug_msg("Valid flag: %u\n", status->states.valid);

	return SI468X_SUCCESS;
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
		} while ((data_in[0] & STATUS0_CTS) == 0);
	}
	else if(type == INTERRUPT)
	{
		while(Si468x_get_int_status());
	}
}

/*
 *
 */
void Si468x_wait_for_stcint(Si468x_wait_type type)
{
	if(type == POLLING)
	{
		do {
			Si468x_rd_reply(0);
		} while ((data_in[0] & STATUS0_STCINT) == 0);
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
    if (sizeof_binary_image(dab_radio_5_0_5_bin) < 2*sizeof(uint8_t)) {
        debug_msg("Error: There's no valid tuner's firmware found. Initialization will be stopped\n");
        return -1;
    }
    
	uint8_t actual_freq;

	// Take the tuner out of reset and wait for 3ms
	Si468x_deassert_reset();
	timer_wait_us(3000);
	// Send power-up and then wait 20us
	Si468x_powerup();
	timer_wait_us(20);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the bootloader image
	Si468x_host_load(&_binary___external_firmwares_rom00_patch_016_bin_start, sizeof_binary_image(rom00_patch_016_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the application image (DAB)
	Si468x_host_load(&_binary___external_firmwares_dab_radio_5_0_5_bin_start, sizeof_binary_image(dab_radio_5_0_5_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Boot the image
	Si468x_boot();

	timer_wait_us(400000);

	Si468x_get_part_info(&Si468x_info_part);

	Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGNAL_LEVEL_THRESHOLD, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_SIGLOW_THRESHOLD, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_DAB_CTRL_DAB_MUTE_ENABLE, 0x0000);
	Si468x_dab_set_property(SI468X_PROP_DAB_DIGRAD_INTERRUPT_SOURCE, 0x0001);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_CFG, 0x0001);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARM, 120);
	Si468x_dab_set_property(SI468X_PROP_DAB_TUNE_FE_VARB, 10);
	Si468x_dab_set_property(SI468X_PROP_PIN_CONFIG_ENABLE, 0x0003);
	Si468x_dab_set_property(SI468X_PROP_DAB_VALID_DETECT_TIME, 2000);

	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_VARM);
	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_VARB);
	Si468x_dab_get_property(SI468X_PROP_DAB_TUNE_FE_CFG);

	Si468x_dab_get_freq_list(&Si468x_freq_list);

	for(actual_freq = 0; actual_freq < Si468x_freq_list.num_freqs; actual_freq++)
	{
		debug_msg("Setting tune frequency to: %u\n", actual_freq);

		Si468x_dab_tune_freq(actual_freq, AUTOMATIC, 0);

		Si468x_dab_digrad_status(1, 0, 1, &Si468x_DAB_status);
	}


	//Si468x_get_digital_service_list();

	return SI468X_SUCCESS;
}

/*
 *
 */
static int Si468x_start_fm()
{
    if (sizeof_binary_image(fmhd_radio_5_0_4_bin) < 2*sizeof(uint8_t)) {
        debug_msg("Error: There's no valid tuner's firmware found. Initialization will be stopped\n");
        return -1;
    }
    
	// Take the tuner out of reset and wait for 3ms
	Si468x_deassert_reset();
	timer_wait_us(3000);
	// Send power-up and then wait 20us
	Si468x_powerup();
	timer_wait_us(20);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the bootloader image
	Si468x_host_load(&_binary___external_firmwares_rom00_patch_016_bin_start, sizeof_binary_image(rom00_patch_016_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the application image (DAB)
	Si468x_host_load(&_binary___external_firmwares_fmhd_radio_5_0_4_bin_start, sizeof_binary_image(fmhd_radio_5_0_4_bin));
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

	Si468x_fm_tune_freq(10280);

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
