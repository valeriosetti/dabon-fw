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
	oled_init();
	debug_msg("Initialization completed\n");
}

FRESULT scan_files(char* path)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            debug_msg("%s\n", fno.fname);
        }
        f_closedir(&dir);
    }

    return res;
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

	/*** Test I2C peripherals (codec & gas-gauge) ***/
/*	// Codec
	uint8_t tmp_addr;
	tmp_addr = 0x0A;
	if (i2c_scan_address(tmp_addr) != I2C_ERROR)
		debug_msg("  Codec found!\n");
	else
		debug_msg("  Codec NOT found\n");

	// Gas-gauge
	tmp_addr = 0x70;
	if (i2c_scan_address(tmp_addr) != I2C_ERROR)
		debug_msg("  Gas-gauge found!\n");
	else
		debug_msg("  Gas-gauge NOT found\n");
*/
	// FatFs test
	FATFS fs;
	if (f_mount(&fs, "\0", 1) == FR_OK) {
		scan_files("/");
	}

	// Timer
	uint32_t curr_time = 0;
	while (1)
	{
		timer_wait_us(1000000UL);
		debug_msg("[%d] card inserted = %d\n", systick_get_tick_count(), sd_card_detect_is_card_inserted());
	}
}
