#include "sdio.h"
#include "stm32f407xx.h"
#include "utils.h"
#include "gpio.h"
#include "clock_configuration.h"
#include "debug_printf.h"
#include "core_cm4.h"
#include "systick.h"

#define debug_msg(...)		debug_printf_with_tag("[SDIO] ", __VA_ARGS__)

static uint32_t SDMMC_GetCmdError(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp1(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint32_t Timeout);
static uint32_t SDMMC_GetCmdResp2(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp3(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp7(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp6(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint16_t *pRCA);

uint32_t SystemCoreClock;

/**
 * Initializes the SDMMC according to the specified
 */
uint32_t SDIO_Init(SDIO_TypeDef *SDIOx, uint32_t WideMode, uint32_t clk_div)
{
	RCC_SDIO_CLK_ENABLE();
	
	uint32_t tmpreg = 	SDIO_CLOCK_EDGE_RISING | SDIO_CLOCK_BYPASS_DISABLE | SDIO_CLOCK_POWER_SAVE_DISABLE |
						WideMode | SDIO_HARDWARE_FLOW_CONTROL_DISABLE | clk_div;

	/* Write to SDMMC CLKCR */
	MODIFY_REG(SDIOx->CLKCR, CLKCR_CLEAR_MASK, tmpreg);  
	
	NVIC_SetPriority(SDIO_IRQn, 0x05);
	NVIC_EnableIRQ(SDIO_IRQn);
	// NOTE: the ISR() is in the "sd_card" module

	return 0;
}

/**
 * 	Initialize GPIOs and DMA
 */
uint32_t SDIO_HwInit()
{
	RCC_GPIOC_CLK_ENABLE();
	RCC_GPIOD_CLK_ENABLE();
	
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
	
	// Configure Rx's DMA
	//	- RX => DMA2, channel 4, stream 3
	RCC_DMA2_CLK_ENABLE();
	CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);	// Disable the DMA before changing its parameters
	
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_CHSEL_Msk, 4UL << DMA_SxCR_CHSEL_Pos);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_DIR_Msk, 0UL << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA2_Stream3->CR, DMA_SxCR_MINC);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_MSIZE_Msk, 2UL << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_PSIZE_Msk, 2UL << DMA_SxCR_PSIZE_Pos);
	SET_BIT(DMA2_Stream3->CR, DMA_SxCR_PFCTRL);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_PL_Msk, 3UL << DMA_SxCR_PL_Pos);
	MODIFY_REG(DMA2_Stream3->FCR, DMA_SxFCR_FTH_Msk, 3UL << DMA_SxFCR_FTH_Pos);
	SET_BIT(DMA2_Stream3->FCR, DMA_SxFCR_DMDIS);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_MBURST_Msk, 1UL << DMA_SxCR_MBURST_Pos);
	MODIFY_REG(DMA2_Stream3->CR, DMA_SxCR_PBURST_Msk, 1UL << DMA_SxCR_PBURST_Pos);
	
	NVIC_SetPriority(DMA2_Stream3_IRQn, 0x06);
	NVIC_EnableIRQ(DMA2_Stream3_IRQn);
	  
	return 0;
}
	
/*===============================================================================
					##### I/O operation functions #####
 ===============================================================================  */
/**
 * Read data (word) from Rx FIFO in blocking mode (polling) 
 */
uint32_t SDIO_ReadFIFO(SDIO_TypeDef *SDIOx)
{
	/* Read data from Rx FIFO */ 
	return (SDIOx->FIFO);
}

/**
 * Write data (word) to Tx FIFO in blocking mode (polling) 
 */
uint32_t SDIO_WriteFIFO(SDIO_TypeDef *SDIOx, uint32_t *pWriteData)
{ 
	/* Write data to FIFO */ 
	SDIOx->FIFO = *pWriteData;

	return 0;
}

/*===============================================================================
					##### Peripheral Control functions #####
 =============================================================================== */
/** 
 * Set SDMMC Power state to ON. 
 */
uint32_t SDIO_PowerState_ON(SDIO_TypeDef *SDIOx)
{  
	/* Set power state to ON */ 
	SDIOx->POWER = SDIO_POWER_PWRCTRL;
	
	return 0;
}

/**
 * Set SDMMC Power state to OFF. 
 */
uint32_t SDIO_PowerState_OFF(SDIO_TypeDef *SDIOx)
{
	/* Set power state to OFF */
	SDIOx->POWER = 0x00000000U;
	
	return 0;
}

/**
 * Get SDMMC Power state. 
 */
uint32_t SDIO_GetPowerState(SDIO_TypeDef *SDIOx)  
{
	return (SDIOx->POWER & SDIO_POWER_PWRCTRL);
}

/**
 * Configure the SDMMC command path according to the specified parameters in
 * SDIO_CmdInitTypeDef structure and send the command 
 */
uint32_t SDIO_SendCommand(SDIO_TypeDef *SDIOx, SDIO_CmdInitTypeDef *Command)
{
	uint32_t tmpreg = 0U;

	/* Set the SDMMC Argument value */
	SDIOx->ARG = Command->Argument;

	/* Set SDMMC command parameters */
	tmpreg |= (uint32_t)(Command->CmdIndex | Command->Response | Command->WaitForInterrupt | Command->CPSM);
	
	/* Write to SDMMC CMD register */
	MODIFY_REG(SDIOx->CMD, CMD_CLEAR_MASK, tmpreg); 
	
	return 0;  
}

/**
 * Return the command index of last command for which response received
 */
uint8_t SDIO_GetCommandResponse(SDIO_TypeDef *SDIOx)
{
	return (uint8_t)(SDIOx->RESPCMD);
}


/**
 * Return the response received from the card for the last command
 */
uint32_t SDIO_GetResponse(SDIO_TypeDef *SDIOx, uint32_t Response)
{
	volatile uint32_t tmp = 0U;
	
	/* Get the response */
	tmp = (uint32_t)&(SDIOx->RESP1) + Response;
	
	return (*(volatile uint32_t *) tmp);
}  

/**
 * Configure the SDMMC data path according to the specified 
 * parameters in the SDIO_DataInitTypeDef.
 */
uint32_t SDIO_ConfigData(SDIO_TypeDef *SDIOx, SDIO_DataInitTypeDef* Data)
{
	uint32_t tmpreg = 0U;

	/* Set the SDMMC Data TimeOut value */
	SDIOx->DTIMER = Data->DataTimeOut;

	/* Set the SDMMC DataLength value */
	SDIOx->DLEN = Data->DataLength;

	/* Set the SDMMC data configuration parameters */
	tmpreg |= (uint32_t)(Data->DataBlockSize | Data->TransferDir | Data->TransferMode | Data->DPSM);
	
	/* Write to SDMMC DCTRL */
	MODIFY_REG(SDIOx->DCTRL, DCTRL_CLEAR_MASK, tmpreg);

	return 0;

}

/**
 * Returns number of remaining data bytes to be transferred.
 */
uint32_t SDIO_GetDataCounter(SDIO_TypeDef *SDIOx)
{
	return (SDIOx->DCOUNT);
}

/**
 * Get the FIFO data
 */
uint32_t SDIO_GetFIFOCount(SDIO_TypeDef *SDIOx)
{
	return (SDIOx->FIFO);
}

/**
 * Sets one of the two options of inserting read wait interval.
 */
uint32_t SDIO_SetSDMMCReadWaitMode(SDIO_TypeDef *SDIOx, uint32_t SDIO_ReadWaitMode)
{
	/* Set SDMMC read wait mode */
	MODIFY_REG(SDIOx->DCTRL, SDIO_DCTRL_RWMOD, SDIO_ReadWaitMode);
	
	return 0;  
}

/*===============================================================================
		 	 ##### Commands management functions #####
 ===============================================================================*/
/**
 * Send the Data Block Lenght command and check the response
 */
uint32_t SDMMC_CmdBlockLength(SDIO_TypeDef *SDIOx, uint32_t BlockSize)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)BlockSize;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SET_BLOCKLEN;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SET_BLOCKLEN, SDIO_CMDTIMEOUT);
	
	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}

	return errorstate;
}

/**
 * Send the Read Single Block command and check the response
 */
uint32_t SDMMC_CmdReadSingleBlock(SDIO_TypeDef *SDIOx, uint32_t ReadAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)ReadAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_READ_SINGLE_BLOCK;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_READ_SINGLE_BLOCK, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}

	return errorstate;
}

/**
 * Send the Read Multi Block command and check the response
 */
uint32_t SDMMC_CmdReadMultiBlock(SDIO_TypeDef *SDIOx, uint32_t ReadAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)ReadAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_READ_MULT_BLOCK;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_READ_MULT_BLOCK, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}

	return errorstate;
}

/**
 * Send the Write Single Block command and check the response
 */
uint32_t SDMMC_CmdWriteSingleBlock(SDIO_TypeDef *SDIOx, uint32_t WriteAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)WriteAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_WRITE_SINGLE_BLOCK;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_WRITE_SINGLE_BLOCK, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}

	return errorstate;
}

/**
 * Send the Write Multi Block command and check the response
 */
uint32_t SDMMC_CmdWriteMultiBlock(SDIO_TypeDef *SDIOx, uint32_t WriteAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)WriteAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_WRITE_MULT_BLOCK;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_WRITE_MULT_BLOCK, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Start Address Erase command for SD and check the response
 */
uint32_t SDMMC_CmdSDEraseStartAdd(SDIO_TypeDef *SDIOx, uint32_t StartAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)StartAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SD_ERASE_GRP_START;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SD_ERASE_GRP_START, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the End Address Erase command for SD and check the response
 */
uint32_t SDMMC_CmdSDEraseEndAdd(SDIO_TypeDef *SDIOx, uint32_t EndAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)EndAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SD_ERASE_GRP_END;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SD_ERASE_GRP_END, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Start Address Erase command and check the response
 */
uint32_t SDMMC_CmdEraseStartAdd(SDIO_TypeDef *SDIOx, uint32_t StartAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)StartAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_ERASE_GRP_START;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_ERASE_GRP_START, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the End Address Erase command and check the response
 */
uint32_t SDMMC_CmdEraseEndAdd(SDIO_TypeDef *SDIOx, uint32_t EndAdd)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = (uint32_t)EndAdd;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_ERASE_GRP_END;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_ERASE_GRP_END, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Erase command and check the response
 */
uint32_t SDMMC_CmdErase(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Set Block Size for Card */ 
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_ERASE;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_ERASE, SDIO_MAXERASETIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Stop Transfer command and check the response.
 */
uint32_t SDMMC_CmdStopTransfer(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD12 STOP_TRANSMISSION  */
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_STOP_TRANSMISSION;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_STOP_TRANSMISSION, 100000000U);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Select Deselect command and check the response.
 */
uint32_t SDMMC_CmdSelDesel(SDIO_TypeDef *SDIOx, uint64_t Addr)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD7 SDMMC_SEL_DESEL_CARD */
	sdmmc_cmdinit.Argument         = (uint32_t)Addr;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SEL_DESEL_CARD;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SEL_DESEL_CARD, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Go Idle State command and check the response.
 */
uint32_t SDMMC_CmdGoIdleState(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_GO_IDLE_STATE;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_NO;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdError(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Operating Condition command and check the response.
 */
uint32_t SDMMC_CmdOperCond(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD8 to verify SD card interface operating condition */
	/* Argument: - [31:12]: Reserved (shall be set to '0')
	- [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
	- [7:0]: Check Pattern (recommended 0xAA) */
	/* CMD Response: R7 */
	sdmmc_cmdinit.Argument         = SDMMC_CHECK_PATTERN;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_HS_SEND_EXT_CSD;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp7(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Application command to verify that that the next command 
 * is an application specific com-mand rather than a standard command
 * and check the response.
 */
uint32_t SDMMC_CmdAppCommand(SDIO_TypeDef *SDIOx, uint32_t Argument)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = (uint32_t)Argument;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_APP_CMD;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	/* If there is a HAL_ERROR, it is a MMC card, else
	it is a SD card: SD card 2.0 (voltage range mismatch)
		 or SD card 1.x */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_APP_CMD, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the command asking the accessed card to send its operating 
 * condition register (OCR)
 */
uint32_t SDMMC_CmdAppOperCommand(SDIO_TypeDef *SDIOx, uint32_t SdType)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = SDMMC_VOLTAGE_WINDOW_SD | SdType;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SD_APP_OP_COND;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp3(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Bus Width command and check the response.
 */
uint32_t SDMMC_CmdBusWidth(SDIO_TypeDef *SDIOx, uint32_t BusWidth)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = (uint32_t)BusWidth;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_APP_SD_SET_BUSWIDTH;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_APP_SD_SET_BUSWIDTH, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Send SCR command and check the response.
 */
uint32_t SDMMC_CmdSendSCR(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD51 SD_APP_SEND_SCR */
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SD_APP_SEND_SCR;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SD_APP_SEND_SCR, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Send CID command and check the response.
 */
uint32_t SDMMC_CmdSendCID(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD2 ALL_SEND_CID */
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_ALL_SEND_CID;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_LONG;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp2(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Send CSD command and check the response.
 */
uint32_t SDMMC_CmdSendCSD(SDIO_TypeDef *SDIOx, uint32_t Argument)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD9 SEND_CSD */
	sdmmc_cmdinit.Argument         = (uint32_t)Argument;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SEND_CSD;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_LONG;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp2(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Send CSD command and check the response.
 */
uint32_t SDMMC_CmdSetRelAdd(SDIO_TypeDef *SDIOx, uint16_t *pRCA)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	/* Send CMD3 SD_CMD_SET_REL_ADDR */
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SET_REL_ADDR;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp6(SDIOx, SDMMC_CMD_SET_REL_ADDR, pRCA);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Status command and check the response.
 */
uint32_t SDMMC_CmdSendStatus(SDIO_TypeDef *SDIOx, uint32_t Argument)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = (uint32_t)Argument;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SEND_STATUS;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SEND_STATUS, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Send the Status register command and check the response.
 */
uint32_t SDMMC_CmdStatusRegister(SDIO_TypeDef *SDIOx)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = 0U;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SD_APP_STATUS;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_SD_APP_STATUS, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Sends host capacity support information and activates the card's 
 * initialization process. Send SDMMC_CMD_SEND_OP_COND command
 */
uint32_t SDMMC_CmdOpCondition(SDIO_TypeDef *SDIOx, uint32_t Argument)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = Argument;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_SEND_OP_COND;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp3(SDIOx);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/**
 * Checks switchable function and switch card function. SDMMC_CMD_HS_SWITCH comand
 */
uint32_t SDMMC_CmdSwitch(SDIO_TypeDef *SDIOx, uint32_t Argument)
{
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;
	uint32_t errorstate = SDMMC_ERROR_NONE;
	
	sdmmc_cmdinit.Argument         = Argument;
	sdmmc_cmdinit.CmdIndex         = SDMMC_CMD_HS_SWITCH;
	sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;
	SDIO_SendCommand(SDIOx, &sdmmc_cmdinit);
	
	/* Check for error conditions */
	errorstate = SDMMC_GetCmdResp1(SDIOx, SDMMC_CMD_HS_SWITCH, SDIO_CMDTIMEOUT);

	if (errorstate != SDMMC_ERROR_NONE) {
		debug_msg("Error in: %s\n", __func__);
	}
	
	return errorstate;
}

/* Private function ----------------------------------------------------------*/  
/**
 * Checks for error conditions for CMD0.
 */
static uint32_t SDMMC_GetCmdError(SDIO_TypeDef *SDIOx)
{
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > SDIO_CMDTIMEOUT) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CMDSENT));
	
	/* Clear all the static flags */
	__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	
	return SDMMC_ERROR_NONE;
}

/**
 * Checks for error conditions for R1 response.
 */
static uint32_t SDMMC_GetCmdResp1(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint32_t Timeout)
{
	uint32_t response_r1;
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > Timeout) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		debug_msg("Error: SDMMC_ERROR_CMD_RSP_TIMEOUT in %s\n", __func__);
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		debug_msg("Error: SDMMC_ERROR_CMD_CRC_FAIL in %s\n", __func__);
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Check response received is of desired command */
	if(SDIO_GetCommandResponse(SDIOx) != SD_CMD)
	{
		debug_msg("Error: SDMMC_ERROR_CMD_CRC_FAIL in %s\n", __func__);
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Clear all the static flags */
	__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	
	/* We have received response, retrieve it for analysis  */
	response_r1 = SDIO_GetResponse(SDIOx, SDIO_RESP1);
	
	if((response_r1 & SDMMC_OCR_ERRORBITS) == SDMMC_ALLZERO)
	{
		return SDMMC_ERROR_NONE;
	}
	else if((response_r1 & SDMMC_OCR_ADDR_OUT_OF_RANGE) == SDMMC_OCR_ADDR_OUT_OF_RANGE)
	{
		debug_msg("Error: SDMMC_ERROR_ADDR_OUT_OF_RANGE in %s\n", __func__);
		return SDMMC_ERROR_ADDR_OUT_OF_RANGE;
	}
	else if((response_r1 & SDMMC_OCR_ADDR_MISALIGNED) == SDMMC_OCR_ADDR_MISALIGNED)
	{
		debug_msg("Error: SDMMC_ERROR_ADDR_MISALIGNED in %s\n", __func__);
		return SDMMC_ERROR_ADDR_MISALIGNED;
	}
	else if((response_r1 & SDMMC_OCR_BLOCK_LEN_ERR) == SDMMC_OCR_BLOCK_LEN_ERR)
	{
		debug_msg("Error: SDMMC_ERROR_BLOCK_LEN_ERR in %s\n", __func__);
		return SDMMC_ERROR_BLOCK_LEN_ERR;
	}
	else if((response_r1 & SDMMC_OCR_ERASE_SEQ_ERR) == SDMMC_OCR_ERASE_SEQ_ERR)
	{
		debug_msg("Error: SDMMC_ERROR_ERASE_SEQ_ERR in %s\n", __func__);
		return SDMMC_ERROR_ERASE_SEQ_ERR;
	}
	else if((response_r1 & SDMMC_OCR_BAD_ERASE_PARAM) == SDMMC_OCR_BAD_ERASE_PARAM)
	{
		debug_msg("Error: SDMMC_ERROR_BAD_ERASE_PARAM in %s\n", __func__);
		return SDMMC_ERROR_BAD_ERASE_PARAM;
	}
	else if((response_r1 & SDMMC_OCR_WRITE_PROT_VIOLATION) == SDMMC_OCR_WRITE_PROT_VIOLATION)
	{
		debug_msg("Error: SDMMC_ERROR_WRITE_PROT_VIOLATION in %s\n", __func__);
		return SDMMC_ERROR_WRITE_PROT_VIOLATION;
	}
	else if((response_r1 & SDMMC_OCR_LOCK_UNLOCK_FAILED) == SDMMC_OCR_LOCK_UNLOCK_FAILED)
	{
		debug_msg("Error: SDMMC_ERROR_LOCK_UNLOCK_FAILED in %s\n", __func__);
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_COM_CRC_FAILED) == SDMMC_OCR_COM_CRC_FAILED)
	{
		debug_msg("Error: SDMMC_ERROR_COM_CRC_FAILED in %s\n", __func__);
		return SDMMC_ERROR_COM_CRC_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_ILLEGAL_CMD) == SDMMC_OCR_ILLEGAL_CMD)
	{
		debug_msg("Error: SDMMC_ERROR_ILLEGAL_CMD in %s\n", __func__);
		return SDMMC_ERROR_ILLEGAL_CMD;
	}
	else if((response_r1 & SDMMC_OCR_CARD_ECC_FAILED) == SDMMC_OCR_CARD_ECC_FAILED)
	{
		debug_msg("Error: SDMMC_ERROR_CARD_ECC_FAILED in %s\n", __func__);
		return SDMMC_ERROR_CARD_ECC_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_CC_ERROR) == SDMMC_OCR_CC_ERROR)
	{
		debug_msg("Error: SDMMC_ERROR_CC_ERR in %s\n", __func__);
		return SDMMC_ERROR_CC_ERR;
	}
	else if((response_r1 & SDMMC_OCR_STREAM_READ_UNDERRUN) == SDMMC_OCR_STREAM_READ_UNDERRUN)
	{
		debug_msg("Error: SDMMC_ERROR_STREAM_READ_UNDERRUN in %s\n", __func__);
		return SDMMC_ERROR_STREAM_READ_UNDERRUN;
	}
	else if((response_r1 & SDMMC_OCR_STREAM_WRITE_OVERRUN) == SDMMC_OCR_STREAM_WRITE_OVERRUN)
	{
		debug_msg("Error: SDMMC_ERROR_STREAM_WRITE_OVERRUN in %s\n", __func__);
		return SDMMC_ERROR_STREAM_WRITE_OVERRUN;
	}
	else if((response_r1 & SDMMC_OCR_CID_CSD_OVERWRITE) == SDMMC_OCR_CID_CSD_OVERWRITE)
	{
		debug_msg("Error: SDMMC_ERROR_CID_CSD_OVERWRITE in %s\n", __func__);
		return SDMMC_ERROR_CID_CSD_OVERWRITE;
	}
	else if((response_r1 & SDMMC_OCR_WP_ERASE_SKIP) == SDMMC_OCR_WP_ERASE_SKIP)
	{
		debug_msg("Error: SDMMC_ERROR_WP_ERASE_SKIP in %s\n", __func__);
		return SDMMC_ERROR_WP_ERASE_SKIP;
	}
	else if((response_r1 & SDMMC_OCR_CARD_ECC_DISABLED) == SDMMC_OCR_CARD_ECC_DISABLED)
	{
		debug_msg("Error: SDMMC_ERROR_CARD_ECC_DISABLED in %s\n", __func__);
		return SDMMC_ERROR_CARD_ECC_DISABLED;
	}
	else if((response_r1 & SDMMC_OCR_ERASE_RESET) == SDMMC_OCR_ERASE_RESET)
	{
		debug_msg("Error: SDMMC_ERROR_ERASE_RESET in %s\n", __func__);
		return SDMMC_ERROR_ERASE_RESET;
	}
	else if((response_r1 & SDMMC_OCR_AKE_SEQ_ERROR) == SDMMC_OCR_AKE_SEQ_ERROR)
	{
		debug_msg("Error: SDMMC_ERROR_AKE_SEQ_ERR in %s\n", __func__);
		return SDMMC_ERROR_AKE_SEQ_ERR;
	}
	else
	{
		debug_msg("Error: SDMMC_ERROR_GENERAL_UNKNOWN_ERR in %s\n", __func__);
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}

/**
 * Checks for error conditions for R2 (CID or CSD) response.
 */
static uint32_t SDMMC_GetCmdResp2(SDIO_TypeDef *SDIOx)
{
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > SDIO_CMDTIMEOUT) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
		
	if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		debug_msg("Error: SDMMC_ERROR_CMD_RSP_TIMEOUT in %s\n", __func__);
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		debug_msg("Error: SDMMC_ERROR_CMD_CRC_FAIL in %s\n", __func__);
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	else
	{
		/* No error flag set */
		/* Clear all the static flags */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	}

	return SDMMC_ERROR_NONE;
}

/**
 * Checks for error conditions for R3 (OCR) response.
 */
static uint32_t SDMMC_GetCmdResp3(SDIO_TypeDef *SDIOx)
{
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > SDIO_CMDTIMEOUT) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		debug_msg("Error: SDMMC_ERROR_CMD_RSP_TIMEOUT in %s\n", __func__);
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else
	{  
		/* Clear all the static flags */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	}
	
	return SDMMC_ERROR_NONE;
}

/**
 * Checks for error conditions for R6 (RCA) response.
 */
static uint32_t SDMMC_GetCmdResp6(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint16_t *pRCA)
{
	uint32_t response_r1;
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > SDIO_CMDTIMEOUT) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		debug_msg("Error: SDMMC_ERROR_CMD_RSP_TIMEOUT in %s\n", __func__);
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		debug_msg("Error: SDMMC_ERROR_CMD_CRC_FAIL in %s\n", __func__);
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Check response received is of desired command */
	if(SDIO_GetCommandResponse(SDIOx) != SD_CMD)
	{	
		debug_msg("Error: SDMMC_ERROR_CMD_CRC_FAIL in %s\n", __func__);
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Clear all the static flags */
	__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	
	/* We have received response, retrieve it.  */
	response_r1 = SDIO_GetResponse(SDIOx, SDIO_RESP1);
	
	if((response_r1 & (SDMMC_R6_GENERAL_UNKNOWN_ERROR | SDMMC_R6_ILLEGAL_CMD | SDMMC_R6_COM_CRC_FAILED)) == SDMMC_ALLZERO)
	{
		*pRCA = (uint16_t) (response_r1 >> 16);
		return SDMMC_ERROR_NONE;
	}
	else if((response_r1 & SDMMC_R6_ILLEGAL_CMD) == SDMMC_R6_ILLEGAL_CMD)
	{
		debug_msg("Error: SDMMC_ERROR_ILLEGAL_CMD in %s\n", __func__);
		return SDMMC_ERROR_ILLEGAL_CMD;
	}
	else if((response_r1 & SDMMC_R6_COM_CRC_FAILED) == SDMMC_R6_COM_CRC_FAILED)
	{
		debug_msg("Error: SDMMC_ERROR_COM_CRC_FAILED in %s\n", __func__);
		return SDMMC_ERROR_COM_CRC_FAILED;
	}
	else
	{
		debug_msg("Error: SDMMC_ERROR_GENERAL_UNKNOWN_ERR in %s\n", __func__);
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}

/**
 * Checks for error conditions for R7 response.
 */
static uint32_t SDMMC_GetCmdResp7(SDIO_TypeDef *SDIOx)
{
	uint32_t start_tick = systick_get_tick_count();
	do {
		if ((systick_get_tick_count() - start_tick) > SDIO_CMDTIMEOUT) {
			debug_msg("Error: SDMMC_ERROR_TIMEOUT in %s\n", __func__);
			return SDMMC_ERROR_TIMEOUT;
		}
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));

	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		/* Card is SD V2.0 compliant */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CMDREND);
		debug_msg("Error: SDMMC_ERROR_CMD_RSP_TIMEOUT in %s\n", __func__);
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CMDREND))
	{
		/* Card is SD V2.0 compliant */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CMDREND);
	}
	
	return SDMMC_ERROR_NONE;
	
}
