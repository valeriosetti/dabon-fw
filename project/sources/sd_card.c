#include "sd_card.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"

// Macros
#define enable_card_detect_tx()			SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS12)
#define disable_card_detect_tx()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR12)
#define is_card_detect_rx_high()		(READ_BIT(GPIOB->IDR, GPIO_IDR_IDR_0) != 0)

/*
 * Initialize the SD card peripheral
 * - PD12 and PB0 for card detect

 */
void sd_card_init()
{
	// Enable the GPIOs' peripheral clock
	RCC_GPIOB_CLK_ENABLE();
	RCC_GPIOC_CLK_ENABLE();
	RCC_GPIOD_CLK_ENABLE();


	// Start with card detect pins PD12 and PB0:
	// - PD12 => output push-pull set high
	// - PB0 => input with pull-down
	disable_card_detect_tx();
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE12_Msk, (MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE12_Pos) );
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_MODER_MODE12_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE12_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE0_Msk, (MODER_INPUT << GPIO_MODER_MODE0_Pos) );
	MODIFY_REG(GPIOB->OSPEEDR, GPIO_MODER_MODE0_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE0_Pos);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD0_Msk, PUPDR_PULL_DOWN << GPIO_PUPDR_PUPD0_Pos);
	enable_card_detect_tx();

	// Configure SDIO pins
	// - PC8 => DAT0 (AF12)
	// - PC9 => DAT1 (AF12)
	// - PC10 => DAT2 (AF12)
	// - PC11 => DAT3 (AF12)
	// - PC12 => CLK (AF12)
	// - PD2 => CMD (AF12)
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODE8_Msk + GPIO_MODER_MODE9_Msk + GPIO_MODER_MODE10_Msk +
				GPIO_MODER_MODE11_Msk + GPIO_MODER_MODE12_Msk,
				(MODER_ALTERNATE << GPIO_MODER_MODE8_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE9_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE10_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE11_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE12_Pos) );
	MODIFY_REG(GPIOD->AFR[1], GPIO_AFRH_AFSEL8_Msk + GPIO_AFRH_AFSEL9_Msk + GPIO_AFRH_AFSEL10_Msk +
				GPIO_AFRH_AFSEL11_Msk + GPIO_AFRH_AFSEL12_Msk,
				(12UL << GPIO_AFRH_AFSEL8_Pos) + (12UL << GPIO_AFRH_AFSEL9_Pos) + (12UL << GPIO_AFRH_AFSEL10_Pos) +
				(12UL << GPIO_AFRH_AFSEL11_Pos) + (12UL << GPIO_AFRH_AFSEL12_Pos));
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODE2_Msk, (MODER_ALTERNATE << GPIO_MODER_MODE2_Pos));
	MODIFY_REG(GPIOD->AFR[0], GPIO_AFRL_AFSEL2_Msk, (12UL << GPIO_AFRL_AFSEL2_Pos));
}

/*
 * Check if the SD card is inserted in polling mode
 */
uint8_t sd_card_is_card_inserted()
{
	return (!is_card_detect_rx_high());
}
