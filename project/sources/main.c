#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"


/*
 *	System's main function
 */
int main(void)
{
	// Configure the main clock
	ClockConfig_SetMainClockAndPrescalers();

	// Test SGTL5000
	i2c_init();
//	#define SGTL5000_I2C_ADDRESS 		0x0A
//	uint8_t reg_addr[2] = {0x00, 0x00};
//	uint8_t rx_data;
//	i2c_write_buffer(SGTL5000_I2C_ADDRESS, reg_addr, 2);
//	i2c_read_buffer(SGTL5000_I2C_ADDRESS, &rx_data, 2);

	uint8_t tmp_addr;
	for (tmp_addr=0; tmp_addr<128; tmp_addr++) {
		if (i2c_scan_address(tmp_addr) != I2C_ERROR)
			while(1);
	}

	// Enable the interrupt from SysTick
	//HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

	// Configure the SysTick
	//	- the counter is reset every 1ms
	//	- interrupt generation is enabled
	//	- the counter is enabled
	//SysTick_Config(168000);
	//NVIC_EnableIRQ(SysTick_IRQn);

	while (1)
	{
	
	}
}
