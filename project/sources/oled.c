#include "oled.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"

#define debug_msg(...)		debug_printf_with_tag("[Oled] ", __VA_ARGS__)

// Macros
#define oled_assert_reset()			SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR9)
#define oled_deassert_reset()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS9)

/*
 *	This initialize the OLED display. Here we assume that the FSMC controller
 *	is already initialized.
 */
void oled_init()
{
	// Configure the reset pin LCD_RES (PD9)
	oled_assert_reset();
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE9_Msk, (MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE9_Pos) );
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_MODER_MODE9_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE9_Pos);
}
