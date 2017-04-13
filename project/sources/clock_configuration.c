#include "stm32f407xx.h"
#include "clock_configuration.h"

/*
 *	Configure main clock and prescalers as follows:
 *		- Clock source = HSE + PLL
 *		- AHB = 168 MHz
 *		- APB1 = 42 MHZ
 *		- APB2 = 84 MHz
 *		- Flash latency = 5 WS
 */
int ClockConfig_SetMainClockAndPrescalers()
{
	uint32_t tmp;

	//__HAL_RCC_PWR_CLK_ENABLE();
	//__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	// Turn on the HSE oscillator and wait for it to stabilize
	RCC->CR |= RCC_CR_HSEON;
	while ((RCC->CR & RCC_CR_HSERDY) == 0);

	// Configure the PLL
	tmp = RCC->PLLCFGR;
	tmp = (tmp & ~RCC_PLLCFGR_PLLQ) | (7UL << 24);  // Q=7
	tmp = (tmp & ~RCC_PLLCFGR_PLLP) | (0UL << 16);  // P=2
	tmp = (tmp & ~RCC_PLLCFGR_PLLN) | (336UL << 6);  // N=168
	tmp = (tmp & ~RCC_PLLCFGR_PLLM) | (HSE_MAIN_DIV << 0); // M=4
	tmp |= RCC_PLLCFGR_PLLSRC;  // HSE is PLL's source
	RCC->PLLCFGR = tmp;

	// Turn on the PLL and wait for it to lock
	RCC->CR |= RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY) == 0);

	// Configure peripherals' dividers
	tmp = RCC->CFGR;
	tmp = (tmp & ~RCC_CFGR_PPRE2_Msk) | RCC_CFGR_PPRE2_DIV2;  // APB2 clock = 84MHz
	tmp = (tmp & ~RCC_CFGR_PPRE1_Msk) | RCC_CFGR_PPRE1_DIV4;  // APB1 clock = 42MHz
	tmp = (tmp & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV1;  // AHB clock = 168MHz (SysTick will be clocked at 21MHz)
	RCC->CFGR = tmp;

	// Configure the Flash latency to 5 wait states (see Table 7 in the Reference Manual)
	tmp = FLASH->ACR;
	tmp = (tmp & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_5WS;
	FLASH->ACR = tmp;

	// Switch to the PLL generated clock
	tmp = RCC->CFGR;
	tmp = (tmp & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
	RCC->CFGR = tmp;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

