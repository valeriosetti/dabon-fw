#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"
#include "uart.h"
#include "debug_printf.h"
#include "output_i2s.h"
#include "spi.h"
#include "timer.h"


/*
 *	System's main function
 */
int main(void)
{
	// Configure the main clock
	ClockConfig_SetMainClockAndPrescalers();

	// Initialize peripherals
	uart_init();
	timer_init();
	i2c_init();
	spi_init();
	output_i2s_init();

	debug_printf("Initialization completed\n");

	/*** Test I2C peripherals (codec & gas-gauge) ***/
	// Codec
	uint8_t tmp_addr;
	tmp_addr = 0x0A;
	if (i2c_scan_address(tmp_addr) != I2C_ERROR)
		debug_printf("  Codec found!\n");
	else
		debug_printf("  Codec NOT found\n");

	// Gas-gauge
	tmp_addr = 0x70;
	if (i2c_scan_address(tmp_addr) != I2C_ERROR)
		debug_printf("  Gas-gauge found!\n");
	else
		debug_printf("  Gas-gauge NOT found\n");

	/*** SPI peripherals (tuner and eeprom) ***/
	// Eeprom
	uint8_t eeprom_id[4];
	spi_set_eeprom_CS();
	timer_wait_us(1);
	spi_read(0x9F, eeprom_id, 4);
	timer_wait_us(1);
	spi_release_eeprom_CS();

	uint8_t index;
	for (index=0; index<sizeof(eeprom_id); index++){
		debug_printf("SPI byte %d = 0x%x\n", index, eeprom_id[index]);
	}

	// Tuner

	// Timer
	uint32_t curr_time = 0;
	while (1)
	{
		timer_wait_us(1000000UL);
		debug_printf("%d\n", curr_time++);
	}
}
