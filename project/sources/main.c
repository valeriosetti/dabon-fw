#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"
#include "uart.h"
#include "debug_printf.h"


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

//	uint8_t tmp_addr;
//	for (tmp_addr=0x0A; tmp_addr<0x80; tmp_addr++) {
//		if (i2c_scan_address(tmp_addr) != I2C_ERROR)
//			while(1);
//	}

	#define STC3115_I2C_ADDRESS 		0x70
	uint8_t reg_addr = 24;
	uint8_t rx_data = 0x00;

	uint8_t tmp;

	while (1)
	{
//		if (i2c_write_buffer(STC3115_I2C_ADDRESS, &reg_addr, 1) == I2C_SUCCESS)
//			i2c_read_buffer(STC3115_I2C_ADDRESS, &rx_data, 1);
		debug_printf("tmp=%d\n", tmp++);
	}
}
