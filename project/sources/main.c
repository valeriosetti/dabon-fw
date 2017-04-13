#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"
#include "uart.h"
#include "debug_printf.h"
#include "output_i2s.h"


/*
 *	System's main function
 */
int main(void)
{
	// Configure the main clock
	ClockConfig_SetMainClockAndPrescalers();

	// Initialize peripherals
	i2c_init();
	uart_init();
	output_i2s_init();

	debug_printf("Initialization completed\n");

	uint8_t tmp_addr;
	volatile uint32_t counter;
	debug_printf("Scanning I2C peripherals:\n");
	for (tmp_addr=0x01; tmp_addr<0x80; tmp_addr++) {
		for(counter=0; counter<0x000A037A; counter++);
		if (i2c_scan_address(tmp_addr) != I2C_ERROR) {
			debug_printf("   >>> Device found at addr = 0x%x <<<\n", tmp_addr);
		} else {
			debug_printf("   No device at addr = 0x%x\n", tmp_addr);
		}
	}

//	#define STC3115_I2C_ADDRESS 		0x70
//	uint8_t reg_addr = 24;
//	uint8_t rx_data = 0x00;

	while (1)
	{
//		if (i2c_write_buffer(STC3115_I2C_ADDRESS, &reg_addr, 1) == I2C_SUCCESS)
//			i2c_read_buffer(STC3115_I2C_ADDRESS, &rx_data, 1);
	}
}
