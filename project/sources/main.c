#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"
#include "uart.h"
#include "debug_printf.h"
#include "output_i2s.h"
#include "spi.h"
#include "timer.h"
#include "tuner.h"
#include "fsmc.h"
#include "oled.h"
#include "sd_card.h"
#include "sd_card_detect.h"
#include "ff.h"
#include "systick.h"
#include "mp3dec.h"
#include "sgtl5000.h"

#define debug_msg(...)		debug_printf_with_tag("[Main] ", __VA_ARGS__)

/*
 * HW peripherals initialization
 */
void HW_init()
{
	// Low level drivers
	uart_init();
	timer_init();
	i2c_init();
	spi_init();
	output_i2s_init();
	fsmc_init();
	SD_Init();
	systick_initialize();

	// Peripherals
	eeprom_init();
	tuner_init();
	sgtl5000_init();
	oled_init();
	debug_msg("Initialization completed\n");
}

/*
 *	System's main function
 */
void main()
{
	// Configure the main clock
	ClockConfig_SetMainClockAndPrescalers();

	// Initialize peripherals
	HW_init();

	// FatFs test
	/*FATFS fs;
	if (f_mount(&fs, "\0", 1) == FR_OK) {
		FILINFO fno;
		DIR dir;
		FRESULT res;
		res = f_opendir(&dir, "/");
		res = f_readdir(&dir, &fno);
		mp3_player_play(fno.fname);
	}*/
	
	sgtl5000_dump_registers();

	// Timer
	uint32_t start_tick = systick_get_tick_count();
	while (1)
	{
		while ((systick_get_tick_count()-start_tick) < 1000UL) {};
		start_tick = systick_get_tick_count();
		//debug_msg("[%d] card inserted = %d\n", systick_get_tick_count(), sd_card_detect_is_card_inserted());
	}
}
