#include "spi.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"

// Macros


/**
 * Initialize the SPI1 peripheral.
 */
void spi_init()
{
	// Enable the GPIOB's peripheral clock
	RCC_GPIOA_CLK_ENABLE();
	RCC_GPIOD_CLK_ENABLE();

	// The following pins will be used for the SPI communications and shared between the tuner and the EEPROM
	// 	- PA5 = CLK (AF5)
	// 	- PA6 = MISO (AF5)
	// 	- PA7 = MOSI (AF5)
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE5_Msk + GPIO_MODER_MODE6_Msk + GPIO_MODER_MODE7_Msk,
				(MODER_ALTERNATE << GPIO_MODER_MODE5_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE6_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE7_Pos));
	MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL5_Msk + GPIO_AFRL_AFSEL6_Msk + GPIO_AFRL_AFSEL7_Msk,
				(5UL << GPIO_AFRL_AFSEL5_Pos) + (5UL << GPIO_AFRL_AFSEL6_Pos) + (5UL << GPIO_AFRL_AFSEL7_Pos) );
	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED5_Msk + GPIO_OSPEEDR_OSPEED6_Msk + GPIO_OSPEEDR_OSPEED7_Msk,
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED5_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED6_Pos) +
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED7_Pos));
	// CS pins are just GPIOs
	// 	- PA0 = CS_EEPROM
	// 	- PD10 = CS_TUNER
	// Both pins are set to '1' at start in order to keep the peripheral not-selected
	SET_BIT(GPIOA->BSRR, GPIO_BSRR_BS0);
	SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS10);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE0_Msk, MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE0_Pos);
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE10_Msk, MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE10_Pos);
	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED0_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED0_Pos);
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_OSPEEDR_OSPEED10_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED10_Pos);

	// Configure the SPI peripheral as
	//  - (0,0) mode
	// 	- master
	//	- CS controlled in software mode (asserted by default)
	//	- clock speed 10.5MHz (even though it slightly exceeds the 10MHz limit of the tuner)
	RCC_SPI1_CLK_ENABLE();
	MODIFY_REG(SPI1->CR1, SPI_CR1_BR_Msk, 2UL << SPI_CR1_BR_Pos);
	SET_BIT(SPI1->CR1, SPI_CR1_MSTR);
	SET_BIT(SPI1->CR1, SPI_CR1_SSM);
	SET_BIT(SPI1->CR1, SPI_CR1_SSI);
	// Enable the SPI peripheral
	SET_BIT(SPI1->CR1, SPI_CR1_SPE);
}

int32_t spi_read(uint8_t* data, uint32_t len)
{
	// Clear the input data register in order to reset the RXNE flag.
	// Warning: data[0] is just used as a temporary variable, but it will be overwritten
	// 		later with useful data!
	*data = SPI1->DR;

	// Read data back
	while (len > 0) {
		SPI1->DR = 0x00;
		while(!(SPI1->SR & SPI_SR_RXNE));
		*data = SPI1->DR;
		data++;
		len--;
	}

	return SPI_SUCCESS;
}

int32_t spi_write(uint8_t* data, uint32_t len)
{
	// Read data back
	while (len > 0) {
		SPI1->DR = *data;
		while(!(SPI1->SR & SPI_SR_TXE));
		data++;
		len--;
	}

	// Wait for the last byte to be completely transmitted
	while(SPI1->SR & SPI_SR_BSY);

	return SPI_SUCCESS;
}
