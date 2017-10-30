#include "oled.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"
#include "fsmc.h"
#include "dabon_logo.h"
#include "font-5x7.h"

#define debug_msg(format, ...)		debug_printf("[oled] " format, ##__VA_ARGS__)

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
#define ENTIRE_DISPLAY_ON										0xA4
#define SET_SEGMENT_REMAP										0xA1
#define SET_COM_SCAN_DIRECTION									0xC8
#define SET_COM_PIN_HW_CONFIGURATION							0xDA
#define SET_CONTRAST_CONTROL									0x81
#define SET_PRECHARGE_PERIOD									0xD9
#define SET_VCOMH												0xDB
#define SET_PAGE_START_ADDRESS									0xB0
#define SET_HIGHER_COLUMN_START_ADDRESS							0x10
#define SET_LOWER_COLUMN_START_ADDRESS							0x00
#define SET_MEMORY_ADDRESSING_METHOD							0x20
	#define PAGE_ADDRESSING_MODE		0x02
	#define HORIZONTAL_ADDRESSING_MODE	0x00
	#define VERTICAL_ADDRESSING_MODE	0x01

// Global variables
uint8_t contrast_level = 0x80;

/*******************************************************************************/
/*	BASIC FUNCTIONS
/*******************************************************************************/
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

	oled_assert_reset();
	// Keep LCD_RES pin low for at least 3us, then release it
	timer_wait_us(5);
	oled_deassert_reset();
	// power on the display
	oled_power_on();

	// Draw the logo
	oled_clear_display();
	oled_draw_image_at_xy(dabon_logo_data, (OLED_WIDTH-dabon_logo_width)/2, 0, dabon_logo_width, dabon_logo_height);
}

/*
 *
 */
static int oled_power_on()
{
	debug_msg("display on\n");
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
	// set memory access method
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_MEMORY_ADDRESSING_METHOD);
	fsmc_write(FSMC_COMMAND_ADDRESS, VERTICAL_ADDRESSING_MODE);
	// set start line address
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_START_LINE);
	// set normal display
	fsmc_write(FSMC_COMMAND_ADDRESS, SET_NORMAL_DISPLAY);
	// set entire display on
	fsmc_write(FSMC_COMMAND_ADDRESS, ENTIRE_DISPLAY_ON);
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

/*
 * Draw a single char on the screen
 */
static void oled_draw_single_char(char ch)
{
	uint16_t col;
	for (col=0; col<FONT_CHAR_WIDTH; col++) {
		fsmc_write(FSMC_DATA_ADDRESS, font_data[FONT_CHAR_WIDTH*(uint16_t)ch + col]);
	}
}

/*******************************************************************************/
/*	DRAWING FUNCTIONS
/*******************************************************************************/
/*
 * Draw the specified image at (x,y) coordinates
 */
void oled_draw_image_at_xy(const uint8_t* img, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
	uint8_t* data_ptr = (uint8_t*)img;
	uint16_t writes_left = width*(height/OLED_VERTICAL_PAGE_SIZE);

	// The vertical dimensions should be expressed in multiples of oled's page vertical size
	y = y/OLED_VERTICAL_PAGE_SIZE;

	oled_set_page_start_address(y);
	oled_set_column_start_address(x);

	while (writes_left > 0) {
		fsmc_write(FSMC_DATA_ADDRESS, *data_ptr);
		data_ptr++;
		writes_left--;
	}
}

/*
 * Clear the entire display
 */
void oled_clear_display()
{
	uint8_t row, col;

	oled_set_page_start_address(0);
	oled_set_column_start_address(0);

	for (col=0; col<OLED_WIDTH; col++) {
		for (row=0; row<(OLED_HEIGTH/OLED_VERTICAL_PAGE_SIZE); row++) {
			fsmc_write(FSMC_DATA_ADDRESS, 0x00);
		}
	}
}

/*
 * Write the specified text starting at (x,y) coordinates
 */
void oled_print_text_at_xy(char* text, uint8_t x, uint8_t y)
{
	oled_set_page_start_address(y);
	oled_set_column_start_address(x);

	uint8_t char_count = 0;

	while ((text != '\0') || (char_count<OLED_MAX_CHARS_IN_LINE)) {
		char ch = *text;
		oled_draw_single_char(ch);
		text++;
		char_count++;
	}
}
