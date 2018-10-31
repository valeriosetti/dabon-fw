#include "systick.h"
#include "stm32f407xx.h"
#include "debug_printf.h"
#include "core_cm4.h"

#define debug_msg(format, ...)		debug_printf("[systick] " format, ##__VA_ARGS__)

uint32_t curr_tick;

/*
 *	Initialize the SysTick timer
 */
void systick_initialize()
{
	SysTick_Config(168000U);
	NVIC_EnableIRQ(SysTick_IRQn);
}

/*
 * 	Get the current SysTick value
 */
uint32_t systick_get_tick_count()
{
	return curr_tick;
}

/*
 * 	
 */
void systick_wait_for_ms(uint32_t delay)
{
	uint32_t start_tick = curr_tick;
	while((curr_tick - start_tick) < delay);
}

/*
 * 	ISR() for SysTick overflow
 */
void SysTick_Handler()
{
	curr_tick++;
}

/*
 * Shell command for reading the current tick count
 */
int systick_gettime(int argc, char *argv[])
{
	debug_msg("Current time = %d\n", systick_get_tick_count());
}
