#include "uart.h"
#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "gpio.h"

// Constants
#define ESC_ASCII_CODE		0x1B

/*
 * Configuration macros
 */
#define UART_DIV_SAMPLING16(_PCLK_, _BAUD_)            (((_PCLK_)*25U)/(4U*(_BAUD_)))
#define UART_DIVMANT_SAMPLING16(_PCLK_, _BAUD_)        (UART_DIV_SAMPLING16((_PCLK_), (_BAUD_))/100U)
#define UART_DIVFRAQ_SAMPLING16(_PCLK_, _BAUD_)        (((UART_DIV_SAMPLING16((_PCLK_), (_BAUD_)) - (UART_DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) * 100U)) * 16U + 50U) / 100U)
#define UART_BRR_SAMPLING16(_PCLK_, _BAUD_)            (((UART_DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) << 4U) + \
                                                        (UART_DIVFRAQ_SAMPLING16((_PCLK_), (_BAUD_)) & 0xF0U)) + \
                                                        (UART_DIVFRAQ_SAMPLING16((_PCLK_), (_BAUD_)) & 0x0FU))

#define UART_DIV_SAMPLING8(_PCLK_, _BAUD_)             (((_PCLK_)*25U)/(2U*(_BAUD_)))
#define UART_DIVMANT_SAMPLING8(_PCLK_, _BAUD_)         (UART_DIV_SAMPLING8((_PCLK_), (_BAUD_))/100U)
#define UART_DIVFRAQ_SAMPLING8(_PCLK_, _BAUD_)         (((UART_DIV_SAMPLING8((_PCLK_), (_BAUD_)) - (UART_DIVMANT_SAMPLING8((_PCLK_), (_BAUD_)) * 100U)) * 8U + 50U) / 100U)
#define UART_BRR_SAMPLING8(_PCLK_, _BAUD_)             (((UART_DIVMANT_SAMPLING8((_PCLK_), (_BAUD_)) << 4U) + \
                                                        ((UART_DIVFRAQ_SAMPLING8((_PCLK_), (_BAUD_)) & 0xF8U) << 1U)) + \
                                                        (UART_DIVFRAQ_SAMPLING8((_PCLK_), (_BAUD_)) & 0x07U))

/*
 * Initialize the UART
 */
int uart_init()
{
	// Enable the GPIOB's peripheral clock
	RCC_GPIOA_CLK_ENABLE();
	// Configure PA2 and PA3 for UART alternate function (AF7)
	GPIOA->MODER |= ((MODER_ALTERNATE << GPIO_MODER_MODE2_Pos) | (MODER_ALTERNATE << GPIO_MODER_MODE3_Pos));
	GPIOA->AFR[0] |= ((7UL << GPIO_AFRL_AFSEL2_Pos) | (7UL << GPIO_AFRL_AFSEL3_Pos));
	GPIOA->OSPEEDR |= ((OSPEEDR_25MHZ << GPIO_OSPEEDR_OSPEED2_Pos) | (OSPEEDR_25MHZ << GPIO_OSPEEDR_OSPEED3_Pos));

	// Enable UART2's clock
	RCC_USART2_CLK_ENABLE();
	// Configure USART2 parameters
	USART2->CR1 |= USART_CR1_UE;
	USART2->BRR = UART_BRR_SAMPLING16(APB1_freq, 115200);

	// Enable transmission
	USART2->CR1 |= USART_CR1_TE;

	// Clear the terminal on the PC side (for a clearer reading)
	uart_put_char(ESC_ASCII_CODE);
	uart_put_char('[');
	uart_put_char('2');
	uart_put_char('J');

	return UART_SUCCECSS;
}

/*
 * Output a single character
 */
int uart_put_char(uint8_t c)
{
	USART2->DR = c;
	while ((USART2->SR & USART_SR_TXE) == 0);
	return UART_SUCCECSS;
}


