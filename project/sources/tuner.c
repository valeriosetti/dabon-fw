#include "tuner.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "timer.h"
#include "debug_printf.h"
#include "utils.h"
#include "clock_configuration.h"
#include "spi.h"

#define debug_msg(...)		debug_printf_with_tag("[Tuner] ", __VA_ARGS__)

// Macros
#define tuner_assert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR8)
#define tuner_deassert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS8)
#define tuner_get_int_status()		READ_BIT(GPIOD->IDR, GPIO_IDR_ID6)

// Private functions
static int tuner_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size);
static int tuner_powerup(void);
static int tuner_rd_reply(void);
static int tuner_load_init(void);
static int tuner_boot(void);

// List of commands
#define TUNER_CMD_RD_REPLY		0x00
#define TUNER_CMD_POWER_UP		0x01
#define TUNER_CMD_HOST_LOAD		0x04
#define TUNER_CMD_LOAD_INIT		0x06
#define TUNER_CMD_BOOT			0x07

// Properties
#define TUNER_XTAL_FREQ						19200000L
#define TUNER_XTAL_CL						10	// in pF - that's taken from the crystal's datasheet!
#define TUNER_XTAL_STARTUP_BIAS_CURRENT		800	// in uA
												// See AN649 at section 9.1.5. It was assumed that:
												// 	- CL=10pF
												//	- startup ESR = 5 x run ESR = 5 x 70ohm = 350ohm (max)


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

	// Take the tuner out of reset and wait for 3ms
	tuner_deassert_reset();
	timer_wait_us(3000);

	// << DEBUG >>
	if (tuner_get_int_status() == 0) {
		debug_msg("Interrupt is already asserted\n");
	} else {
		debug_msg("Interrupt is not asserted\n");
	}
	// Send power-up and then wait 20us
	debug_msg("power-up\n");
	tuner_powerup();
	timer_wait_us(20);
	// << DEBUG >>
	if (tuner_get_int_status() == 0) {
		debug_msg("Interrupt is now asserted\n");
	} else {
		debug_msg("Interrupt is still not asserted\n");
	}

	tuner_boot();	// DEBUG
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

	if (data_in != NULL) {
		int i;
		for (i=0; i<data_in_size; i++) {
			debug_msg("  rsp byte %d = 0x%x\n", i, data_in[i]);
		}
	}
}

/*
 *
 */
static int tuner_powerup()
{
	uint8_t data_out[16] = {TUNER_CMD_POWER_UP,
							0x80,	// toggle interrupt when CTS is available
							0x17,	// external crystal; TR_SIZE=0x7 (see AN649 at section 9.1)
							(TUNER_XTAL_STARTUP_BIAS_CURRENT/10),	// see comments in the define
							((TUNER_XTAL_FREQ & 0x000000FF)>>0),
							((TUNER_XTAL_FREQ & 0x0000FF00)>>8),
							((TUNER_XTAL_FREQ & 0x00FF0000)>>16),
							((TUNER_XTAL_FREQ & 0xFF000000)>>24),
							(2*TUNER_XTAL_CL/0.381),	// see AN649 at section 9.3,
							0x10,	// fixed
							0x00,	// fixed
							0x00,	// fixed
							0x00,	// fixed
							((TUNER_XTAL_STARTUP_BIAS_CURRENT/10)/2),	// see AN649 at section 9.2
							0x00,	// fixed
							0x00,	// fixed
							};
	uint8_t data_in[4];

	debug_msg("powerup\n");
	tuner_send_cmd(data_out, array_size(data_out), data_in, array_size(data_in));
}

/*
 *
 */
static int tuner_rd_reply()
{
	uint8_t data_in[6];

	debug_msg("rd_reply\n");
	tuner_send_cmd(NULL, 0, data_in, array_size(data_in));
}

/*
 *
 */
static int tuner_boot()
{
	uint8_t data_out[2] = {TUNER_CMD_BOOT, 0x00};
	uint8_t data_in[4];

	debug_msg("boot\n");
	tuner_send_cmd(data_out, array_size(data_out), data_in, array_size(data_in));
}

/*
 *
 */
static int tuner_load_init()
{
	uint8_t data_out[2] = {TUNER_CMD_LOAD_INIT, 0x00};
	uint8_t data_in[4];

	debug_msg("load_init\n");
	tuner_send_cmd(data_out, array_size(data_out), data_in, array_size(data_in));
}
