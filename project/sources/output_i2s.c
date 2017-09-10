#include "output_i2s.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "clock_configuration.h"
#include "utils.h"
#include "debug_printf.h"

/**************************************************************************************************
 **************************************************************************************************
 **************************************************************************************************/
// Typedefs and enums
typedef struct {
		uint32_t sample_freq;
		uint32_t N;
		uint32_t R;
		uint32_t DIV;
		uint32_t ODD;
} I2S_PLL_CONFIG;

// Global variables and structures
I2S_PLL_CONFIG 	i2s_pll_configurations[] = {
		{.sample_freq=8000, .N=256, .R=5, .DIV=12, .ODD=1},
		{.sample_freq=16000, .N=213., .R=2, .DIV=13, .ODD=0},
		{.sample_freq=22050, .N=429, .R=4, .DIV=9, .ODD=1},
		{.sample_freq=32000, .N=213, .R=2, .DIV=6, .ODD=1},
		{.sample_freq=44100, .N=271, .R=2, .DIV=6, .ODD=0},
		{.sample_freq=48000, .N=258, .R=3, .DIV=3, .ODD=1},
		{.sample_freq=96000, .N=344, .R=2, .DIV=3, .ODD=1},
};
#define PLL_CONFIGURATIONS_COUNT  		(sizeof(i2s_pll_configurations)/sizeof(I2S_PLL_CONFIG))

// Output buffers
#define	OUTPUT_BUFFER_SIZE		1000UL
uint32_t output_buffer[OUTPUT_BUFFER_SIZE];

// Macros
#define I2S3_enable()		do{ SET_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SE);	} while(0)
#define I2S3_disable()		do{ CLEAR_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SE);	} while(0)

/**************************************************************************************************
 **************************************************************************************************
 **************************************************************************************************/
/*
 * Initialize the I2S peripheral
 */
int output_i2s_init()
{
	int ret_val;

	// Enable the GPIOB's peripheral clock
	RCC_GPIOA_CLK_ENABLE();
	RCC_GPIOB_CLK_ENABLE();
	RCC_GPIOC_CLK_ENABLE();

	// Configure pins:
	//	- PA15 = I2S3_WS (AF6)
	//	- PB3 = I2S3_CK (AF6)
	// 	- PB5 = I2S3_SD (AF6)
	// 	- PC7 = I2S3_MCK (AF6)
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE15_Msk, MODER_ALTERNATE << GPIO_MODER_MODE15_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE3_Msk, MODER_ALTERNATE << GPIO_MODER_MODE3_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE5_Msk, MODER_ALTERNATE << GPIO_MODER_MODE5_Pos);
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODE7_Msk, MODER_ALTERNATE << GPIO_MODER_MODE7_Pos);
	MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL15_Msk, 6UL << GPIO_AFRH_AFSEL15_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL3_Msk, 6UL << GPIO_AFRL_AFSEL3_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL5_Msk, 6UL << GPIO_AFRL_AFSEL5_Pos);
	MODIFY_REG(GPIOC->AFR[0], GPIO_AFRL_AFSEL7_Msk, 6UL << GPIO_AFRL_AFSEL7_Pos);
	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED15_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED15_Pos);
	MODIFY_REG(GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED3_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED3_Pos);
	MODIFY_REG(GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED5_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED5_Pos);
	MODIFY_REG(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED7_Msk, OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED7_Pos);

	// Configure the input clock for the I2S3 peripheral
	ret_val = output_i2s_ConfigurePLL(44100);
	if (ret_val < OUTPUT_I2S_SUCCESS)
		return ret_val;

	// Set SPI3 to I2S transmit mode
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 2UL << SPI_I2SCFGR_I2SCFG_Pos);
	// Enable the DMA request on I2S3
	SET_BIT(SPI3->CR2, SPI_CR2_TXDMAEN);
	// Enable the I2S peripheral
	I2S3_enable();

	// Configure the DMA (DMA 1, stream 7, channel 0):
	//	- no peripheral or memory burst modes
	//	- priority = very high
	//	- memory data size = 16bit
	//	- peripheral data size = 16bit
	//	- memory increment enabled (peripheral one is not!)
	//	- circular mode (that's ONLY FOR DEBUG!)
	RCC_DMA1_CLK_ENABLE();
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_CHSEL_Msk, 0UL << DMA_SxCR_CHSEL_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_PL_Msk, 3UL << DMA_SxCR_PL_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_MSIZE_Msk, 1UL << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_PSIZE_Msk, 1UL << DMA_SxCR_PSIZE_Pos);
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_MINC);
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_CIRC);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_DIR_Msk, 1UL << DMA_SxCR_DIR_Pos);
	// Set the DMA source and destination addresses
	DMA1_Stream7->NDTR = OUTPUT_BUFFER_SIZE;
	DMA1_Stream7->PAR = (uint32_t) &(SPI3->DR);
	DMA1_Stream7->M0AR = (uint32_t) output_buffer;
	// Enable the DMA
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_EN);

	return OUTPUT_I2S_SUCCESS;
}

/*
 * Configure the PLL
 */
int output_i2s_ConfigurePLL(uint32_t samplig_freq)
{
	// Look for the specified sample rate frequency in the list of allowed configurations
	uint8_t index = 0;
	while ( (index<PLL_CONFIGURATIONS_COUNT) && (i2s_pll_configurations[index].sample_freq != samplig_freq) ) {
		index++;
	}
	// If no matching configuration was found then return an error
	if (index == PLL_CONFIGURATIONS_COUNT) {
		return OUTPUT_I2S_PLL_CONFIG_NOT_FOUND;
	}

	// Configure the input PLL
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SR_Msk, i2s_pll_configurations[index].R << RCC_PLLI2SCFGR_PLLI2SR_Pos);
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN_Msk, i2s_pll_configurations[index].N << RCC_PLLI2SCFGR_PLLI2SN_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLI2SON);
	while(READ_BIT(RCC->CR, RCC_CR_PLLI2SRDY) == 0);

	// Enable I2S3's peripheral clock
	RCC_SPI3_CLK_ENABLE();

	// Set the SPI3 peripheral to I2S mode
	SET_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SMOD);
	// Configure the I2S's input clock divider
	MODIFY_REG(SPI3->I2SPR, SPI_I2SPR_I2SDIV_Msk, i2s_pll_configurations[index].DIV << SPI_I2SPR_I2SDIV_Pos);
	MODIFY_REG(SPI3->I2SPR, SPI_I2SPR_ODD_Msk, i2s_pll_configurations[index].ODD << SPI_I2SPR_ODD_Pos);
	// Output also the MCLK
	SET_BIT(SPI3->I2SPR, SPI_I2SPR_MCKOE);

	return OUTPUT_I2S_SUCCESS;
}

