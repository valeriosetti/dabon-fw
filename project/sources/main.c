#include "stm32f407xx.h"
#include "clock_configuration.h"


/*
 *	System's main function
 */
int main(void)
{
	// Configure the main clock
	ClockConfig_SetMainClockAndPrescalers();

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
