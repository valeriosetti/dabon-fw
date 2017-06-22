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

// Private functions for commands
static int Si468x_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size);
static int Si468x_rd_reply(uint32_t extra_data_len);
static int Si468x_powerup(void);
static int Si468x_load_init(void);
static int Si468x_host_load(uint8_t* img_data, uint32_t len);
static int Si468x_boot(void);
static int Si468x_get_sys_state(void);
// Private functions for advanced management
static int Si468x_wait_for_cts(void);
static int Si468x_start_dab(void);


// List of commands for DAB mode
#define SI468X_CMD_RD_REPLY							0x00
#define SI468X_CMD_POWER_UP						    0x01
#define SI468X_CMD_HOST_LOAD					    0x04
#define SI468X_CMD_FLASH_LOAD					    0x05
#define SI468X_CMD_LOAD_INIT					    0x06
#define SI468X_CMD_BOOT							    0x07
#define SI468X_CMD_GET_SYS_STATE				    0x09
#define GET_POWER_UP_ARGS                           0x0A
#define SI468X_READ_OFFSET                          0x10
#define SI468X_GET_FUNC_INFO                        0x12
#define SI468X_SET_PROPERTY                         0x13
#define SI468X_GET_PROPERTY                         0x14
#define SI468X_WRITE_STORAGE                        0x15
#define SI468X_READ_STORAGE                         0x16
#define SI468X_GET_DIGITAL_SERVICE_LIST             0x80
#define SI468X_START_DIGITAL_SERVICE                0x81
#define SI468X_STOP_DIGITAL_SERVICE                 0x82
#define SI468X_GET_DIGITAL_SERVICE_DATA             0x84
#define SI468X_DAB_TUNE_FREQ                        0xB0
#define SI468X_DAB_DIGRAD_STATUS                    0xB2
#define SI468X_DAB_GET_EVENT_STATUS                 0xB3
#define SI468X_DAB_GET_ENSEMBLE_INFO                0xB4
#define SI468X_DAB_GET_SERVICE_LINKING_INFO         0xB7
#define SI468X_DAB_SET_FREQ_LIST                    0xB8
#define SI468X_DAB_GET_FREQ_LIST                    0xB9
#define SI468X_DAB_GET_COMPONENT_INFO               0xBB
#define SI468X_DAB_GET_TIME                         0xBC
#define SI468X_DAB_GET_AUDIO_INFO                   0xBD
#define SI468X_DAB_GET_SUBCHAN_INFO                 0xBE
#define SI468X_DAB_GET_FREQ_INFO                    0xBF
#define SI468X_DAB_GET_SERVICE_INFO                 0xC0
#define SI468X_TEST_GET_RSSI                        0xE5
#define SI468X_DAB_TEST_GET_BER_INFO                0xE8

// List of properties for DAB mode
#define INT_CTL_REPEAT								0x0001
#define DIGITAL_IO_OUTPUT_SELECT                    0x0200
#define DIGITAL_IO_OUTPUT_SAMPLE_RATE               0x0201
#define DIGITAL_IO_OUTPUT_FORMAT                    0x0202
#define DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_1        0x0203
#define DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_2        0x0204
#define DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_3        0x0205
#define DIGITAL_IO_OUTPUT_FORMAT_OVERRIDES_4        0x0206
#define AUDIO_ANALOG_VOLUME                         0x0300
#define AUDIO_MUTE                                  0x0301
#define AUDIO_OUTPUT_CONFIG                         0x0302
#define PIN_CONFIG_ENABLE                           0x0800
#define WAKE_TONE_ENABLE                            0x0900
#define WAKE_TONE_PERIOD                            0x0901
#define WAKE_TONE_FREQ                              0x0902
#define WAKE_TONE_AMPLITUDE                         0x0903
#define DAB_TUNE_FE_VARM                            0x1710
#define DAB_TUNE_FE_VARB                            0x1711
#define DAB_TUNE_FE_CFG                             0x1712
#define DIGITAL_SERVICE_INT_SOURCE                  0x8100
#define DIGITAL_SERVICE_RESTART_DELAY               0x8101
#define DAB_DIGRAD_INTERRUPT_SOURCE                 0xB000
#define DAB_DIGRAD_RSSI_HIGH_THRESHOLD              0xB001
#define DAB_DIGRAD_RSSI_LOW_THRESHOLD               0xB002
#define DAB_VALID_RSSI_TIME                         0xB200
#define DAB_VALID_RSSI_THRESHOLD                    0xB201
#define DAB_VALID_ACQ_TIME                          0xB202
#define DAB_VALID_SYNC_TIME                         0xB203
#define DAB_VALID_DETECT_TIME                       0xB204
#define DAB_EVENT_INTERRUPT_SOURCE                  0xB300
#define DAB_EVENT_MIN_SVRLIST_PERIOD                0xB301
#define DAB_EVENT_MIN_SVRLIST_PERIOD_RECONFIG       0xB302
#define DAB_EVENT_MIN_FREQINFO_PERIOD               0xB303
#define DAB_XPAD_ENABLE                             0xB400
#define DAB_DRC_OPTION                              0xB401
#define DAB_CTRL_DAB_MUTE_ENABLE                    0xB500
#define DAB_CTRL_DAB_MUTE_SIGNAL_LEVEL_THRESHOLD    0xB501
#define DAB_CTRL_DAB_MUTE_WIN_THRESHOLD             0xB502
#define DAB_CTRL_DAB_UNMUTE_WIN_THRESHOLD           0xB503
#define DAB_CTRL_DAB_MUTE_SIGLOSS_THRESHOLD         0xB504
#define DAB_CTRL_DAB_MUTE_SIGLOW_THRESHOLD          0xB505
#define DAB_TEST_BER_CONFIG                         0xE800














































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

// Tuner's firmware
//extern uint8_t _binary___external_firmwares_fmhd_radio_5_0_4_bin_start;
//extern uint8_t _binary___external_firmwares_fmhd_radio_5_0_4_bin_end;
extern uint8_t _binary___external_firmwares_dab_radio_5_0_5_bin_start;
extern uint8_t _binary___external_firmwares_dab_radio_5_0_5_bin_end;
extern uint8_t _binary___external_firmwares_rom00_patch_016_bin_start;
extern uint8_t _binary___external_firmwares_rom00_patch_016_bin_end;

#define sizeof_binary_image(_img_name_)		(uint32_t)((&_img_name_##_end)-(&_img_name_##_start))

// status register bits
#define PUP_STATE_mask				0xC0
#define PUP_STATE_BOOTLOADER		0x80
#define PUP_STATE_APPLICATION		0xC0
#define STATUS0_CTS					0x80
#define STATUS0_ERRCMD				0x40

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
	/*if (data_in != NULL) {
		int i;
		for (i=0; i<data_in_size; i++) {
			debug_msg("  rsp byte %d = 0x%x\n", i, data_in[i]);
		}
	}*/
}

/********************************************************************************
 * COMMANDS FUNCTIONS
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
	Si468x_wait_for_cts();

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

	Si468x_wait_for_cts();
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

		Si468x_wait_for_cts();

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

	Si468x_wait_for_cts();

	// data_in[3] contains informations about the current device's state
	if ((data_in[3] & PUP_STATE_mask) != PUP_STATE_APPLICATION)
		debug_msg("boot failure\n");
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

/********************************************************************************
 * ADVANCED FUNCTIONS
 ********************************************************************************/
/*
 *
 */
static int Si468x_wait_for_cts()
{
	do {
		Si468x_rd_reply(0);
	} while ((data_in[0] & STATUS0_CTS) == 0);
}

/*
 *
 */
static int Si468x_start_dab()
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
	Si468x_host_load(&_binary___external_firmwares_rom00_patch_016_bin_start,
			sizeof_binary_image(_binary___external_firmwares_rom00_patch_016_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	Si468x_load_init();
	// Send the application image (DAB)
	Si468x_host_load(&_binary___external_firmwares_dab_radio_5_0_5_bin_start,
			sizeof_binary_image(_binary___external_firmwares_dab_radio_5_0_5_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Boot the image
	Si468x_boot();
}
