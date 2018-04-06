#include "output_i2s.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "clock_configuration.h"
#include "utils.h"
#include "debug_printf.h"
#include "systick.h"
#include "string.h"
#include "kernel.h"

#define debug_msg(format, ...)		debug_printf("[output_i2s] " format, ##__VA_ARGS__)

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

// DMA buffers
#define DMA_BUFFERS_SIZE	    (2048)
int16_t dma_buffers[2][2*DMA_BUFFERS_SIZE];

// Output (circular) buffer which holds samples before being copied to the DMA ones
#define OUTPUT_BUFFER_SIZE      (2048)
struct {
    int16_t samples[2*OUTPUT_BUFFER_SIZE];
    uint16_t start_index;
    uint16_t stop_index;
    int16_t count;
} output_buffer;

// Callback function to signal that the buffer has been freed
void (*free_buff_space_callback)(void);

// Interrupt handling task
ALLOCATE_TASK(output_i2s, 1);

// Macros
#define I2S3_enable()		do{ SET_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SE);	} while(0)
#define I2S3_disable()		do{ CLEAR_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SE);	} while(0)

/*********************************************************************************************/
/*		PUBLIC FUNCTIONS
/*********************************************************************************************/
/*
 * Initialize the I2S peripheral
 */
int32_t output_i2s_init()
{
	int ret_val;
    
    kernel_init_task(&output_i2s_task);
    
    // Configure buffers
    output_buffer.count = 0;
    output_buffer.start_index = 0;
    output_buffer.stop_index = 0;
    memset(output_buffer.samples, 0, sizeof(output_buffer.samples));
    memset(dma_buffers[0], 0, DMA_BUFFERS_SIZE*sizeof(int16_t));
    memset(dma_buffers[1], 0, DMA_BUFFERS_SIZE*sizeof(int16_t));

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
	ret_val = output_i2s_ConfigurePLL(48000);
	if (ret_val != 0) {
		debug_msg("Error in: %s\n", __func__);
		return ret_val;
	}

	// Configure the DMA (DMA 1, stream 7, channel 0):
	//	- no peripheral or memory burst modes
	//	- priority = very high
	//	- memory data size = 16bit
	//	- peripheral data size = 16bit
	//	- memory increment enabled (peripheral one is not!)
	//	- double buffer mode
	RCC_DMA1_CLK_ENABLE();
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_CHSEL_Msk, 0UL << DMA_SxCR_CHSEL_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_PL_Msk, 3UL << DMA_SxCR_PL_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_MSIZE_Msk, 1UL << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_PSIZE_Msk, 1UL << DMA_SxCR_PSIZE_Pos);
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_MINC);
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_DBM);
	MODIFY_REG(DMA1_Stream7->CR, DMA_SxCR_DIR_Msk, 1UL << DMA_SxCR_DIR_Pos);
	// Set the DMA source and destination addresses
	DMA1_Stream7->NDTR = DMA_BUFFERS_SIZE;
	DMA1_Stream7->PAR = (uint32_t) &(SPI3->DR);
	DMA1_Stream7->M0AR = (uint32_t) dma_buffers[0];
	DMA1_Stream7->M1AR = (uint32_t) dma_buffers[1];
	// Enable DMA's interrupt
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_TCIE);
	NVIC_SetPriority(DMA1_Stream7_IRQn, 0x01);
	NVIC_EnableIRQ(DMA1_Stream7_IRQn);
	// Enable the DMA
	SET_BIT(DMA1_Stream7->CR, DMA_SxCR_EN);

	return 0;
}

/*
 * Configure the PLL
 */
int32_t output_i2s_ConfigurePLL(uint32_t samplig_freq)
{
	// Look for the specified sample rate frequency in the list of allowed configurations
	uint8_t index = 0;
	while ( (index<PLL_CONFIGURATIONS_COUNT) && (i2s_pll_configurations[index].sample_freq != samplig_freq) ) {
		index++;
	}
	// If no matching configuration was found then return an error
	if (index == PLL_CONFIGURATIONS_COUNT) {
		debug_msg("Error: PLL configuration not found\n");
		return -1;
	}

	// Disable the I2S peripheral and also its clock
	I2S3_disable();
	RCC_SPI3_CLK_DISABLE();
	// Disable the I2S's PLL
	CLEAR_BIT(RCC->CR, RCC_CR_PLLI2SON);
	// Configure the input PLL
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SR_Msk, i2s_pll_configurations[index].R << RCC_PLLI2SCFGR_PLLI2SR_Pos);
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN_Msk, i2s_pll_configurations[index].N << RCC_PLLI2SCFGR_PLLI2SN_Pos);
	// Re-enable the PLL
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
	// Set SPI3 to I2S transmit mode
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 2UL << SPI_I2SCFGR_I2SCFG_Pos);
	// Enable the DMA request on I2S3
	SET_BIT(SPI3->CR2, SPI_CR2_TXDMAEN);
	// Enable the I2S peripheral
	I2S3_enable();

	return 0;
}

/*
 * Enqueue samples into the local buffer (if there's enough space available)
 */
int32_t output_i2s_enqueue_samples(int16_t* data, uint16_t samples_count)
{
	// check if there's enough space to store incoming samples
	if (samples_count > (OUTPUT_BUFFER_SIZE-output_buffer.count))
		return -1;

	uint16_t data_to_copy;
	while (samples_count > 0) {
		if (output_buffer.stop_index + samples_count < OUTPUT_BUFFER_SIZE) {
			data_to_copy = samples_count;
			memcpy(&output_buffer.samples[output_buffer.stop_index], data, data_to_copy*sizeof(int16_t));
			output_buffer.stop_index += data_to_copy;
		} else {
			data_to_copy = (OUTPUT_BUFFER_SIZE-1) - output_buffer.stop_index;
			memcpy(&output_buffer.samples[output_buffer.stop_index], data, data_to_copy*sizeof(int16_t));
			output_buffer.stop_index = 0;
		}
		samples_count -= data_to_copy;
		data += data_to_copy;
		output_buffer.count += data_to_copy;
	}

	return 0;
}

/*
 * Return the free space of the local buffer
 */
uint32_t output_i2s_get_buffer_free_space()
{
	return (OUTPUT_BUFFER_SIZE - output_buffer.count);
}

/*
 * Register a callback function which should be called when the local buffer is
 * partially freed.
 * A NULL input parameters clears the callback.
 */
void output_i2s_register_callback(void (*func)(void))
{
	free_buff_space_callback = func;
}

/*********************************************************************************************/
/*		INTERRUPT HANDLING
/*********************************************************************************************/
/*
 * ISR - This interrupt is triggered every time the DMA completes the buffer transmission
 */
void DMA1_Stream7_IRQHandler(void)
{
    if (DMA1->HISR & (DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)) {
		debug_msg("Error in DMA transfer\n");
	}
	DMA1->HIFCR = (DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7);

    kernel_activate_task_immediately(&output_i2s_task);
}

/*
 * This task copies data from the input buffer to the DMA's ones
 */
int32_t output_i2s_task_func(void* arg)
{
    // Update the data on the buffer which is idle (if playing 1 then update 0 and viceversa)
    int16_t* dma_ptr = (READ_BIT(DMA1_Stream7->CR, DMA_SxCR_CT)) ? dma_buffers[0] : dma_buffers[1];
    uint16_t remaining_data = DMA_BUFFERS_SIZE;
    uint16_t data_to_copy;
    
    while (remaining_data > 0) {
        // Check if there's something valid inside the output_buffer, otherwise fill the remaining
        // space with 0
        if (output_buffer.count > 0) {
        	// compute the amount of data that should be copied
        	data_to_copy = (output_buffer.count<DMA_BUFFERS_SIZE) ? output_buffer.count : DMA_BUFFERS_SIZE;
        	// check if the copy can be performed with a single operation or in multiple steps
        	if (output_buffer.start_index + data_to_copy - 1 < OUTPUT_BUFFER_SIZE) {
        		// do not limit data_to_copy in this case
        		memcpy(dma_ptr, &output_buffer.samples[output_buffer.start_index], data_to_copy*sizeof(int16_t));
        		output_buffer.start_index += data_to_copy;
        	} else {
        		data_to_copy = OUTPUT_BUFFER_SIZE - output_buffer.start_index;
        		memcpy(dma_ptr, &output_buffer.samples[output_buffer.start_index], data_to_copy*sizeof(int16_t));
        		output_buffer.start_index = 0;
        	}
        	dma_ptr += data_to_copy;
        	output_buffer.count -= data_to_copy;
        	remaining_data -= data_to_copy;
        } else {
            memset(dma_ptr, 0, remaining_data*sizeof(int16_t));
            remaining_data = 0;
        }
    }

	if (free_buff_space_callback != NULL)
		(*free_buff_space_callback)();

    return WAIT_FOR_RESUME;
}
