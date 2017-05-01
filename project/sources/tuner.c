#include "tuner.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "timer.h"
#include "debug_printf.h"
#include "utils.h"
#include "clock_configuration.h"
#include "spi.h"
#include "string.h"

#define debug_msg(...)		debug_printf_with_tag("[Tuner] ", __VA_ARGS__)

// Macros
#define tuner_assert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR8)
#define tuner_deassert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS8)
#define tuner_get_int_status()		READ_BIT(GPIOD->IDR, GPIO_IDR_ID6)

// Private functions for commands
static int tuner_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size);
static int tuner_rd_reply(uint32_t extra_data_len);
static int tuner_powerup(void);
static int tuner_load_init(void);
static int tuner_host_load(uint8_t* img_data, uint32_t len);
static int tuner_boot(void);
static int tuner_get_sys_state(void);
// Private functions for advanced management
static int tuner_wait_for_cts(void);
static int tuner_start_dab(void);


// List of commands
#define TUNER_CMD_RD_REPLY		0x00
#define TUNER_CMD_POWER_UP		0x01
#define TUNER_CMD_HOST_LOAD		0x04
#define TUNER_CMD_LOAD_INIT		0x06
#define TUNER_CMD_BOOT			0x07
#define TUNER_CMD_GET_SYS_STATE	0x09

// Properties
#define TUNER_XTAL_FREQ						19200000L
#define TUNER_XTAL_CL						10	// in pF - that's taken from the crystal's datasheet!
#define TUNER_XTAL_STARTUP_BIAS_CURRENT		800	// in uA
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
void tuner_init()
{
	// Configure GPIOs
	//	PD8 -> RSTB (output, push-pull, low at startup)
	//	PD6 -> INT (input with pull-up)
	RCC_GPIOD_CLK_ENABLE();
	tuner_assert_reset();
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE8_Msk, MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE8_Pos);
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_MODER_MODE8_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE8_Pos);
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE6_Msk, MODER_INPUT << GPIO_MODER_MODE6_Pos);
	MODIFY_REG(GPIOD->PUPDR, GPIO_PUPDR_PUPD6_Msk, PUPDR_PULL_UP << GPIO_PUPDR_PUPD6_Pos);
}

/*
 *
 */
static int tuner_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size)
{
	spi_set_tuner_CS();
	timer_wait_us(1);
	if (data_out != NULL)
		spi_write(data_out, data_out_size);
	if (data_in != NULL)
		spi_read(data_in, data_in_size);
	timer_wait_us(1);
	spi_release_tuner_CS();

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
static int tuner_rd_reply(uint32_t extra_data_len)
{
	data_out[0] = TUNER_CMD_RD_REPLY;
	tuner_send_cmd(data_out, 1, data_in, 4+extra_data_len);
}

/*
 *
 */
static int tuner_powerup()
{
	data_out[0] = TUNER_CMD_POWER_UP;
	data_out[1] = 0x80;	// toggle interrupt when CTS is available
	data_out[2] = 0x17;	// external crystal; TR_SIZE=0x7 (see AN649 at section 9.1)
	data_out[3] = (TUNER_XTAL_STARTUP_BIAS_CURRENT/10);	// see comments in the define
	data_out[4] = ((TUNER_XTAL_FREQ & 0x000000FF)>>0);
	data_out[5] = ((TUNER_XTAL_FREQ & 0x0000FF00)>>8);
	data_out[6] = ((TUNER_XTAL_FREQ & 0x00FF0000)>>16);
	data_out[7] = ((TUNER_XTAL_FREQ & 0xFF000000)>>24);
	data_out[8] = (2*TUNER_XTAL_CL/0.381);	// see AN649 at section 9.3,
	data_out[9] = 0x10;	// fixed
	data_out[10] = 0x00;	// fixed
	data_out[11] = 0x00;	// fixed
	data_out[12] = 0x00;	// fixed
	data_out[13] = ((TUNER_XTAL_STARTUP_BIAS_CURRENT/10)/2);	// see AN649 at section 9.2
	data_out[14] = 0x00;	// fixed
	data_out[15] = 0x00;	// fixed

	tuner_send_cmd(data_out, 16, NULL, 0);
	tuner_wait_for_cts();

	// data_in[3] contains informations about the current device's state
	if ((data_in[3] & PUP_STATE_mask) != PUP_STATE_BOOTLOADER)
		debug_msg("powerup failure\n");
}

/*
 *
 */
static int tuner_load_init()
{
	data_out[0] = TUNER_CMD_LOAD_INIT;
	data_out[1] = 0x00;

	tuner_send_cmd(data_out, 2, NULL, 0);

	tuner_wait_for_cts();
}

/*
 *
 */
static int tuner_host_load(uint8_t* img_data, uint32_t len)
{
	uint32_t curr_len;

	// Each command can send up to 4096 bytes. Therefore, if the sent
	// image is bigger, then split it into consecutive chucks
	do {
		data_out[0] = TUNER_CMD_HOST_LOAD;
		data_out[1] = 0x00;
		data_out[2] = 0x00;
		data_out[3] = 0x00;

		curr_len = (len > 4096) ? 4096 : len;

		spi_set_tuner_CS();
		timer_wait_us(1);
		spi_write(data_out, 4);
		spi_write(img_data, curr_len);
		timer_wait_us(1);
		spi_release_tuner_CS();

		tuner_wait_for_cts();

		len -= curr_len;
		img_data += curr_len;
	} while(len > 0);
}

/*
 *
 */
static int tuner_boot()
{
	data_out[0] = TUNER_CMD_BOOT;
	data_out[1] = 0x00;

	tuner_send_cmd(data_out, 2, NULL, 0);

	tuner_wait_for_cts();

	// data_in[3] contains informations about the current device's state
	if ((data_in[3] & PUP_STATE_mask) != PUP_STATE_APPLICATION)
		debug_msg("boot failure\n");
}

/*
 *
 */
static int tuner_get_sys_state()
{
	data_out[0] = TUNER_CMD_GET_SYS_STATE;
	data_out[1] = 0x00;

	tuner_send_cmd(data_out, 2, NULL, 0);

	tuner_rd_reply(2);

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
static int tuner_wait_for_cts()
{
	do {
		tuner_rd_reply(0);
	} while ((data_in[0] & STATUS0_CTS) == 0);
}

/*
 *
 */
static int tuner_start_dab()
{
	// Take the tuner out of reset and wait for 3ms
	tuner_deassert_reset();
	timer_wait_us(3000);
	// Send power-up and then wait 20us
	tuner_powerup();
	timer_wait_us(20);
	// Begin firmware loading phase
	tuner_load_init();
	// Send the bootloader image
	tuner_host_load(&_binary___external_firmwares_rom00_patch_016_bin_start,
			sizeof_binary_image(_binary___external_firmwares_rom00_patch_016_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Begin firmware loading phase
	tuner_load_init();
	// Send the application image (DAB)
	tuner_host_load(&_binary___external_firmwares_dab_radio_5_0_5_bin_start,
			sizeof_binary_image(_binary___external_firmwares_dab_radio_5_0_5_bin));
	// Wait for 4ms
	timer_wait_us(4000);
	// Boot the image
	tuner_boot();
}
