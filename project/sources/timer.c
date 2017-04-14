#include "timer.h"
#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "utils.h"

/**
 * Initialize the TIM2 (32bit counter) peripheral which will be used to generate
 * accurate time delays
 */
void timer_init()
{
	RCC_TIM2_CLK_ENABLE();

	// Since APB1 is running at 42MHz, but APB1 prescaler is >1, the timer's input clock is
	// 84 MHz. As a consequence we divide it by 84 (=83+1) in order to increase the timer
	// every 1us.
	TIM2->PSC = 83UL;
	SET_BIT(TIM2->EGR, TIM_EGR_UG);
	// Start the timer
	SET_BIT(TIM2->CR1, TIM_CR1_CEN);
}

/**
 *	Wait for the specified amount of microseconds in polling mode. Being TIM2 a 32bit counter,
 *	the longest time the user can wait for is 4294 seconds (more than 1 day!)
 */
void timer_wait_us(uint32_t us_delay)
{
	uint32_t start_value = TIM2->CNT;
	while( (TIM2->CNT - start_value) < us_delay);
}
