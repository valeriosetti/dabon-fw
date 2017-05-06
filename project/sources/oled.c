#include "oled.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"
#include "fsmc.h"

#define debug_msg(...)		debug_printf_with_tag("[Oled] ", __VA_ARGS__)

// Macros
#define oled_assert_reset()			SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR9)
#define oled_deassert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS9)

// Private functions
static int oled_power_on(void);
static int oled_power_off(void);

// Commands
#define SET_DISPLAY_ON											0xAF
#define SET_DISPLAY_OFF											0xAE
#define CHARGE_PUMP_SETTING										0x8D
#define DISPLAY_CLOCK_DIVIDE_RATIO_OSCILLATION_FREQUENCY		0xD5
#define SET_MULTIPLEX_RATIO										0xA8
#define SET_DISPLAY_OFFSET										0xD3
#define SET_START_LINE											0x40
#define SET_NORMAL_DISPLAY										0xA6
#define ENTIRE_DISPLAY_OFF										0xA4
#define SET_SEGMENT_REMAP										0xA1
#define SET_COM_SCAN_DIRECTION									0xC8
#define SET_COM_PIN_HW_CONFIGURATION							0xDA
#define SET_CONTRAST_CONTROL									0x81
#define SET_PRECHARGE_PERIOD									0xD9
#define SET_VCOMH												0xDB
#define SET_PAGE_START_ADDRESS									0xB0
#define SET_HIGHER_COLUMN_START_ADDRESS							0x10
#define SET_LOWER_COLUMN_START_ADDRESS							0x00

// Global variables
uint8_t contrast_level = 0x80;

/*
 *	This initialize the OLED display. Here we assume that the FSMC controller
 *	is already initialized.
 */
void oled_init()
{
	// Configure the reset pin LCD_RES (PD9). It will be set low at startup
	oled_assert_reset();
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE9_Msk, (MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE9_Pos) );
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_MODER_MODE9_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE9_Pos);

	// Keep LCD_RES pin low for at least 3us, then release it
	timer_wait_us(5);
	oled_deassert_reset();
	// power on the display
	oled_power_on();
}

/*
 *
 */
static int oled_power_on()
{
	debug_msg("display on");
	// turn panel off
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_DISPLAY_OFF);
	// set display clock divide ratio/oscillator frequency
	fsmc_write(FSMC_COMMAND_ADDRESS, DISPLAY_CLOCK_DIVIDE_RATIO_OSCILLATION_FREQUENCY);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x80);
	// set multiplex ratio (1/64)
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_MULTIPLEX_RATIO);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x3F);
	// set diplay offset (no offset)
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_DISPLAY_OFFSET);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x00);
	// enable charge pump
	fsmc_write(FSMC_COMMAND_ADDRESS, CHARGE_PUMP_SETTING);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x14);
	// set start line address
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_START_LINE);
	// set normal display
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_NORMAL_DISPLAY);
	// set entire display off
	fsmc_write(FSMC_COMMAND_ADDRESS, ENTIRE_DISPLAY_OFF);
	// set segment re-map
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_SEGMENT_REMAP);
	// set scan direction
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_COM_SCAN_DIRECTION);
	// set COM pins hardware configuration
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_COM_PIN_HW_CONFIGURATION);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x12);
	// set contrast control
	oled_set_contrast(contrast_level);
	// set pre-charge period
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_PRECHARGE_PERIOD);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0xF1);
	// set VCOMH
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_VCOMH);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x40);
	// turn on oled panel
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_DISPLAY_ON);

	timer_wait_us(100000);
}

/*
 *
 */
static int oled_power_off()
{
	debug_msg("display off");
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_DISPLAY_OFF);
	debug_msg("charge pump disable");
	fsmc_write(FSMC_COMMAND_ADDRESS, CHARGE_PUMP_SETTING);
	fsmc_write(FSMC_COMMAND_ADDRESS, 0x10);
	debug_msg("waiting 100ms");
	timer_wait_us(100000);
}

/*
 *
 */
void oled_set_page_start_address(uint8_t add)
{
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_PAGE_START_ADDRESS | (add & 0x7));
}

/*
 *
 */
void oled_set_column_start_address(uint8_t add)
{
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_HIGHER_COLUMN_START_ADDRESS | (add >> 4));
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_LOWER_COLUMN_START_ADDRESS | (add & 0xF));
}

/*
 *
 */
void oled_set_contrast(uint8_t value)
{
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_CONTRAST_CONTROL);
	fsmc_write(FSMC_COMMAND_ADDRESS, value);
}
