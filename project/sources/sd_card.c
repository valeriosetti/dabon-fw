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
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_OSPEEDR_OSPEED12_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED12_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE0_Msk, (MODER_INPUT << GPIO_MODER_MODE0_Pos) );
	MODIFY_REG(GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED0_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED0_Pos);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD0_Msk, PUPDR_PULL_DOWN << GPIO_PUPDR_PUPD0_Pos);
	enable_card_detect_tx();

	// Configure SDIO pins
	// - PC8 => DAT0 (AF12 + pull-up)
	// - PC9 => DAT1 (AF12 + pull-up)
	// - PC10 => DAT2 (AF12 + pull-up)
	// - PC11 => DAT3 (AF12 + pull-up)
	// - PC12 => CLK (AF12 + pull-up)
	// - PD2 => CMD (AF12 + pull-up)
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODE8_Msk + GPIO_MODER_MODE9_Msk + GPIO_MODER_MODE10_Msk +
				GPIO_MODER_MODE11_Msk + GPIO_MODER_MODE12_Msk,
				(MODER_ALTERNATE << GPIO_MODER_MODE8_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE9_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE10_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE11_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE12_Pos) );
	MODIFY_REG(GPIOC->PUPDR, GPIO_PUPDR_PUPD8_Msk + GPIO_PUPDR_PUPD9_Msk + GPIO_PUPDR_PUPD10_Msk +
				GPIO_PUPDR_PUPD11_Msk + GPIO_PUPDR_PUPD12_Msk,
				(PUPDR_PULL_UP << GPIO_PUPDR_PUPD8_Pos) + (PUPDR_PULL_UP << GPIO_PUPDR_PUPD9_Pos) +
				(PUPDR_PULL_UP << GPIO_PUPDR_PUPD10_Pos) + (PUPDR_PULL_UP << GPIO_PUPDR_PUPD11_Pos) +
				(PUPDR_PULL_UP << GPIO_PUPDR_PUPD12_Pos));
	MODIFY_REG(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED8_Msk + GPIO_OSPEEDR_OSPEED9_Msk + GPIO_OSPEEDR_OSPEED10_Msk +
				GPIO_OSPEEDR_OSPEED11_Msk + GPIO_OSPEEDR_OSPEED12_Msk,
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED8_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED9_Pos) +
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED10_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED11_Pos) +
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED12_Pos));
	MODIFY_REG(GPIOC->AFR[1], GPIO_AFRH_AFSEL8_Msk + GPIO_AFRH_AFSEL9_Msk + GPIO_AFRH_AFSEL10_Msk +
				GPIO_AFRH_AFSEL11_Msk + GPIO_AFRH_AFSEL12_Msk,
				(12UL << GPIO_AFRH_AFSEL8_Pos) + (12UL << GPIO_AFRH_AFSEL9_Pos) + (12UL << GPIO_AFRH_AFSEL10_Pos) +
				(12UL << GPIO_AFRH_AFSEL11_Pos) + (12UL << GPIO_AFRH_AFSEL12_Pos));
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE2_Msk, (MODER_ALTERNATE << GPIO_MODER_MODE2_Pos));
	MODIFY_REG(GPIOD->PUPDR, GPIO_PUPDR_PUPD2_Msk, PUPDR_PULL_UP << GPIO_PUPDR_PUPD2_Pos);
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_OSPEEDR_OSPEED2_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED2_Pos);
	MODIFY_REG(GPIOD->AFR[0], GPIO_AFRL_AFSEL2_Msk, (12UL << GPIO_AFRL_AFSEL2_Pos));

	// Enable SDIO's and DMAs' clocks
	RCC_SDIO_CLK_ENABLE();
	RCC_DMA2_CLK_ENABLE();

	// Configure DMAs
	//	- TX => DMA2, channel 4, stream 6
	//	- RX => DMA2, channel 4, stream 3



	/* Configure DMA Rx parameters
	  dmaRxHandle.Init.Channel             = SD_DMAx_Rx_CHANNEL;
	  dmaRxHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	  dmaRxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
	  dmaRxHandle.Init.MemInc              = DMA_MINC_ENABLE;
	  dmaRxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	  dmaRxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	  dmaRxHandle.Init.Mode                = DMA_PFCTRL;
	  dmaRxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	  dmaRxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	  dmaRxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	  dmaRxHandle.Init.MemBurst            = DMA_MBURST_INC4;
	  dmaRxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;*/

	/* Configure DMA Tx parameters
	  dmaTxHandle.Init.Channel             = SD_DMAx_Tx_CHANNEL;
	  dmaTxHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	  dmaTxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
	  dmaTxHandle.Init.MemInc              = DMA_MINC_ENABLE;
	  dmaTxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	  dmaTxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	  dmaTxHandle.Init.Mode                = DMA_PFCTRL;
	  dmaTxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	  dmaTxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	  dmaTxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	  dmaTxHandle.Init.MemBurst            = DMA_MBURST_INC4;
	  dmaTxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;*/
}

/*
 * Check if the SD card is inserted in polling mode
 */
uint8_t sd_card_is_card_inserted()
{
	return (!is_card_detect_rx_high());
}
