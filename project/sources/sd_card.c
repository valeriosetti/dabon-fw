#include "sd_card.h"
#include "sd_card_detect.h"
#include "systick.h"

/* Private functions ---------------------------------------------------------*/
static uint32_t SD_PowerON(void);                      
static uint32_t _SD_InitCard(void);
//~ static uint32_t SD_SendStatus(uint32_t *pCardStatus);
static uint32_t SD_WideBus_Enable(void);
static uint32_t SD_WideBus_Disable(void);
static uint32_t SD_FindSCR(uint32_t *pSCR);
//~ static uint32_t SD_PowerOFF(void);
//~ static uint32_t SD_Write_IT(void);
//~ static uint32_t SD_Read_IT(void);
//~ static void SD_DMATransmitCplt(DMA_HandleTypeDef *hdma);
//~ static void SD_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
//~ static void SD_DMAError(DMA_HandleTypeDef *hdma);
//~ static void SD_DMATxAbort(DMA_HandleTypeDef *hdma);
//~ static void SD_DMARxAbort(DMA_HandleTypeDef *hdma);

/* Global variables ---------------------------------------------------------*/
SD_HandleTypeDef sd_handle;
	
/*==============================================================================
					##### Initialization and de-initialization functions #####
  ==============================================================================*/
/*
 * Initializes the SD according to the specified parameters in the 
 * SD_HandleTypeDef and create the associated handle.
 */
uint32_t SD_Init()
{
	SDIO_HwInit();
	sd_card_detect_init();
	
	sd_handle.Context = SD_CONTEXT_NONE;
	
	return 0;
}

/*
 * Initializes the SD Card.
 * This function initializes the SD card. It could be used when a card 
 * re-initialization is needed.
 */
uint32_t SD_InitCard()
{
	uint32_t errorstate = 0;

	/* Initialize SDIO peripheral interface with default configuration */
	SDIO_Init(SDIO, SDIO_BUS_WIDE_1B);
	/* Disable SDIO Clock */
	SD_DISABLE(sd_handle); 
	/* Set Power State to ON */
	SDIO_PowerState_ON(SDIO);
	/* Enable SDIO Clock */
	SD_ENABLE(sd_handle);
	/* Required power up waiting time before starting the SD initialization  sequence */
	systick_wait_for_ms(2U);
	/* Identify card operating voltage */
	errorstate = SD_PowerON();
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* Card initialization */
	errorstate = _SD_InitCard();
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}

	return 0;
}

/*==============================================================================
				##### IO operation functions #####
==============================================================================  */
/*
 * Reads block(s) from a specified address in a card. The Data transfer is managed by DMA mode. 
 */
//int32_t SD_ReadBlocks_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks)
//~ {
	//SDIO_DataInitTypeDef config;
	//uint32_t errorstate = SD_ERROR_NONE;
	
	//if(sd_handle.State == SD_STATE_READY)
	//{
		//sd_handle.ErrorCode = DMA_ERROR_NONE;
		
		//if((BlockAdd + NumberOfBlocks) > (sd_handle.SdCard.LogBlockNbr)) {
			//sd_handle.ErrorCode |= SD_ERROR_ADDR_OUT_OF_RANGE;
			//return -1;
		//}
		//sd_handle.State = SD_STATE_BUSY;
		///* Initialize data control register */
		//SDIO->DCTRL = 0U;
		
//#ifdef SDIO_STA_STBITER
		//SD_ENABLE_IT((SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_DATAEND | SDIO_IT_STBITERR));
//#else /* SDIO_STA_STBITERR not defined */
		//SD_ENABLE_IT((SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_DATAEND));
//#endif /* SDIO_STA_STBITERR */
		
		///////////////// FIXME ///////////////
		///* Set the DMA transfer complete callback */
		////sd_handle.hdmarx->XferCpltCallback = SD_DMAReceiveCplt;
		///* Set the DMA error callback */
		////sd_handle.hdmarx->XferErrorCallback = SD_DMAError;
		///* Set the DMA Abort callback */
		////sd_handle.hdmarx->XferAbortCallback = NULL;
		///* Enable the DMA Channel */
		////DMA_Start_IT(sd_handle.hdmarx, (uint32_t)&SDIO->FIFO, (uint32_t)pData, (uint32_t)(BLOCKSIZE * NumberOfBlocks)/4);
		///* Enable SD DMA transfer */
		////SD_DMA_ENABLE(hsd);
		
		//if(sd_handle.SdCard.CardType != CARD_SDHC_SDXC) {
			//BlockAdd *= 512U;
		//}
		
		///* Configure the SD DPSM (Data Path State Machine) */ 
		//config.DataTimeOut   = SDMMC_DATATIMEOUT;
		//config.DataLength    = BLOCKSIZE * NumberOfBlocks;
		//config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
		//config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
		//config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
		//config.DPSM          = SDIO_DPSM_ENABLE;
		//SDIO_ConfigData(SDIO, &config);
		///* Set Block Size for Card */ 
		//errorstate = SDMMC_CmdBlockLength(SDIO, BLOCKSIZE);
		//if(errorstate != SD_ERROR_NONE) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(sd_handle, SDIO_STATIC_FLAGS); 
			//sd_handle.ErrorCode |= errorstate;
			//sd_handle.State = SD_STATE_READY;
			//return -2;
		//}
		///* Read Blocks in DMA mode */
		//if(NumberOfBlocks > 1U) {
			//sd_handle.Context = (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA);
			///* Read Multi Block command */ 
			//errorstate = SDMMC_CmdReadMultiBlock(SDIO, BlockAdd);
		//} else {
			//sd_handle.Context = (SD_CONTEXT_READ_SINGLE_BLOCK | SD_CONTEXT_DMA);
			///* Read Single Block command */ 
			//errorstate = SDMMC_CmdReadSingleBlock(SDIO, BlockAdd);
		//}
		//if(errorstate != SD_ERROR_NONE) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
			//sd_handle.ErrorCode |= errorstate;
			//sd_handle.State = SD_STATE_READY;
			//return -3;
		//}
		//return 0;
	//}
	//else
	//{
		//return 1;	// busy
	//}
//}

/*
 * Writes block(s) to a specified address in a card. The Data transfer is managed by DMA mode. 
 */
////int32_t SD_WriteBlocks_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks)
//~ {
	//SDIO_DataInitTypeDef config;
	//uint32_t errorstate = SD_ERROR_NONE;
	
	//if(sd_handle.State == SD_STATE_READY)
	//{
		//sd_handle.ErrorCode = DMA_ERROR_NONE;
		
		//if((BlockAdd + NumberOfBlocks) > (sd_handle.SdCard.LogBlockNbr)) {
			//sd_handle.ErrorCode |= SD_ERROR_ADDR_OUT_OF_RANGE;
			//return -1;
		//}
		//sd_handle.State = SD_STATE_BUSY;
		///* Initialize data control register */
		//SDIO->DCTRL = 0U;
		
		///* Enable SD Error interrupts */  
//#ifdef SDIO_STA_STBITER
		//SD_ENABLE_IT((SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_TXUNDERR | SDIO_IT_STBITERR));    
//#else /* SDIO_STA_STBITERR not defined */
		//SD_ENABLE_IT((SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_TXUNDERR));    
//#endif /* SDIO_STA_STBITERR */
		
		///////////// FIXME /////////////
		///* Set the DMA transfer complete callback */
		////sd_handle.hdmatx->XferCpltCallback = SD_DMATransmitCplt;
		///* Set the DMA error callback */
		////sd_handle.hdmatx->XferErrorCallback = SD_DMAError;
		///* Set the DMA Abort callback */
		////sd_handle.hdmatx->XferAbortCallback = NULL;
		
		//if(sd_handle.SdCard.CardType != CARD_SDHC_SDXC) {
			//BlockAdd *= 512U;
		//}
		///* Set Block Size for Card */ 
		//errorstate = SDMMC_CmdBlockLength(SDIO, BLOCKSIZE);
		//if(errorstate != SD_ERROR_NONE) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
			//sd_handle.ErrorCode |= errorstate;
			//sd_handle.State = SD_STATE_READY;
			//return -2;
		//}
		
		///* Write Blocks in Polling mode */
		//if(NumberOfBlocks > 1U) {
			//sd_handle.Context = (SD_CONTEXT_WRITE_MULTIPLE_BLOCK | SD_CONTEXT_DMA);
			///* Write Multi Block command */ 
			//errorstate = SDMMC_CmdWriteMultiBlock(SDIO, BlockAdd);
		//} else {
			//sd_handle.Context = (SD_CONTEXT_WRITE_SINGLE_BLOCK | SD_CONTEXT_DMA);
			///* Write Single Block command */
			//errorstate = SDMMC_CmdWriteSingleBlock(SDIO, BlockAdd);
		//}
		//if(errorstate != SD_ERROR_NONE) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
			//sd_handle.ErrorCode |= errorstate;
			//sd_handle.State = SD_STATE_READY;
			//return -3;
		//}
		
		///* Enable SDIO DMA transfer */
		////SD_DMA_ENABLE(hsd);    ///// FIXME //////
		///* Enable the DMA Channel */
		////DMA_Start_IT(sd_handle.hdmatx, (uint32_t)pData, (uint32_t)&SDIO->FIFO, (uint32_t)(BLOCKSIZE * NumberOfBlocks)/4);
		
		///* Configure the SD DPSM (Data Path State Machine) */ 
		//config.DataTimeOut   = SDMMC_DATATIMEOUT;
		//config.DataLength    = BLOCKSIZE * NumberOfBlocks;
		//config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
		//config.TransferDir   = SDIO_TRANSFER_DIR_TO_CARD;
		//config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
		//config.DPSM          = SDIO_DPSM_ENABLE;
		//SDIO_ConfigData(SDIO, &config);
		
		//return 0;
	//}
	//else
	//{
		//return 1;	// busy
	//}
//~ }

/*
 * Erases the specified memory area of the given SD card.
 */
//int32_t SD_Erase(uint32_t BlockStartAdd, uint32_t BlockEndAdd)
//~ {
	//uint32_t errorstate = SD_ERROR_NONE;
	
	//if(sd_handle.State == SD_STATE_READY)
	//{
		//sd_handle.ErrorCode = DMA_ERROR_NONE;
		//if(BlockEndAdd < BlockStartAdd) {
			//sd_handle.ErrorCode |= SD_ERROR_PARAM;
			//return -1;
		//}
		
		//if(BlockEndAdd > (sd_handle.SdCard.LogBlockNbr)) {
			//sd_handle.ErrorCode |= SD_ERROR_ADDR_OUT_OF_RANGE;
			//return -2;
		//}
		
		//sd_handle.State = SD_STATE_BUSY;
		///* Check if the card command class supports erase command */
		//if(((sd_handle.SdCard.Class) & SDIO_CCCC_ERASE) == 0U) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
			//sd_handle.ErrorCode |= SD_ERROR_REQUEST_NOT_APPLICABLE;
			//sd_handle.State = SD_STATE_READY;
			//return -3;
		//}
		//if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);  
			//sd_handle.ErrorCode |= SD_ERROR_LOCK_UNLOCK_FAILED;
			//sd_handle.State = SD_STATE_READY;
			//return -4;
		//}
		///* Get start and end block for high capacity cards */
		//if(sd_handle.SdCard.CardType != CARD_SDHC_SDXC) {
			//BlockStartAdd *= 512U;
			//BlockEndAdd   *= 512U;
		//}
		///* According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and erase_group_end(CMD33) */
		//if(sd_handle.SdCard.CardType != CARD_SECURED) {
			///* Send CMD32 SD_ERASE_GRP_START with argument as addr  */
			//errorstate = SDMMC_CmdSDEraseStartAdd(SDIO, BlockStartAdd);
			//if(errorstate != SD_ERROR_NONE) {
				///* Clear all the static flags */
				//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
				//sd_handle.ErrorCode |= errorstate;
				//sd_handle.State = SD_STATE_READY;
				//return -5;
			//}
			///* Send CMD33 SD_ERASE_GRP_END with argument as addr  */
			//errorstate = SDMMC_CmdSDEraseEndAdd(SDIO, BlockEndAdd);
			//if(errorstate != SD_ERROR_NONE) {
				///* Clear all the static flags */
				//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
				//sd_handle.ErrorCode |= errorstate;
				//sd_handle.State = SD_STATE_READY;
				//return -6;
			//}
		//}
		///* Send CMD38 ERASE */
		//errorstate = SDMMC_CmdErase(SDIO);
		//if(errorstate != SD_ERROR_NONE) {
			///* Clear all the static flags */
			//SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
			//sd_handle.ErrorCode |= errorstate;
			//sd_handle.State = SD_STATE_READY;
			//return -7;
		//}
		//sd_handle.State = SD_STATE_READY;
		
		//return 0;
	//}
	//else
	//{
		//return 1;	// busy
	//}
//}

/*
 * This function handles SD card interrupt request
 */
////void SD_IRQHandler()
//~ {
	//~ uint32_t errorstate = SD_ERROR_NONE;
	
	//~ /* Check for SDIO interrupt flags */
	//~ if(SD_GET_FLAG(SDIO_IT_DATAEND) != RESET)
	//~ {
		//~ SD_CLEAR_FLAG(SDIO_FLAG_DATAEND); 
		
//~ #ifdef SDIO_STA_STBITERR
		//~ SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT|\
														 //~ SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR | SDIO_IT_STBITERR);
//~ #else /* SDIO_STA_STBITERR not defined */
		//~ SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT|\
														 //~ SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR);
//~ #endif
		
		//~ if((sd_handle.Context & SD_CONTEXT_IT) != RESET)
		//~ {
			//~ if(((sd_handle.Context & SD_CONTEXT_READ_MULTIPLE_BLOCK) != RESET) || ((sd_handle.Context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != RESET))
			//~ {
				//~ errorstate = SDMMC_CmdStopTransfer(SDIO);
				//~ if(errorstate != SD_ERROR_NONE)
				//~ {
					//~ sd_handle.ErrorCode |= errorstate;
					//~ SD_ErrorCallback(hsd);
				//~ }
			//~ }
			
			//~ /* Clear all the static flags */
			//~ SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
			
			//~ sd_handle.State = SD_STATE_READY;
			//~ if(((sd_handle.Context & SD_CONTEXT_READ_SINGLE_BLOCK) != RESET) || ((sd_handle.Context & SD_CONTEXT_READ_MULTIPLE_BLOCK) != RESET))
			//~ {
				//~ SD_RxCpltCallback(hsd);
			//~ }
			//~ else
			//~ {
				//~ SD_TxCpltCallback(hsd);
			//~ }
		//~ }
		//~ else if((sd_handle.Context & SD_CONTEXT_DMA) != RESET)
		//~ {
			//~ if((sd_handle.Context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != RESET)
			//~ {
				//~ errorstate = SDMMC_CmdStopTransfer(SDIO);
				//~ if(errorstate != SD_ERROR_NONE)
				//~ {
					//~ sd_handle.ErrorCode |= errorstate;
					//~ SD_ErrorCallback(hsd);
				//~ }
			//~ }
			//~ if(((sd_handle.Context & SD_CONTEXT_READ_SINGLE_BLOCK) == RESET) && ((sd_handle.Context & SD_CONTEXT_READ_MULTIPLE_BLOCK) == RESET))
			//~ {
				//~ /* Disable the DMA transfer for transmit request by setting the DMAEN bit
				//~ in the SD DCTRL register */
				//~ SDIO->DCTRL &= (uint32_t)~((uint32_t)SDIO_DCTRL_DMAEN);
				
				//~ sd_handle.State = SD_STATE_READY;
				
				//~ SD_TxCpltCallback(hsd);
			//~ }
		//~ }
	//~ }
	
	//~ else if(SD_GET_FLAG(SDIO_IT_TXFIFOHE) != RESET)
	//~ {
		//~ SD_CLEAR_FLAG(SDIO_FLAG_TXFIFOHE);
		
		//~ SD_Write_IT(hsd);
	//~ }
	
	//~ else if(SD_GET_FLAG(SDIO_IT_RXFIFOHF) != RESET)
	//~ {
		//~ SD_CLEAR_FLAG(SDIO_FLAG_RXFIFOHF);
		
		//~ SD_Read_IT(hsd);
	//~ }
	
//~ #ifdef SDIO_STA_STBITERR
	//~ else if(SD_GET_FLAG(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR | SDIO_IT_STBITERR) != RESET)
	//~ {
		//~ /* Set Error code */
		//~ if(SD_GET_FLAG(SDIO_IT_DCRCFAIL) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_DATA_CRC_FAIL; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_DTIMEOUT) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_DATA_TIMEOUT; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_RXOVERR) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_RX_OVERRUN; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_TXUNDERR) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_TX_UNDERRUN; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_STBITERR) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_DATA_TIMEOUT;
		//~ }

		//~ /* Clear All flags */
		//~ SD_CLEAR_FLAG(SDIO_STATIC_FLAGS | SDIO_FLAG_STBITERR);
		
		//~ /* Disable all interrupts */
		//~ SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT|\
														 //~ SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR |SDIO_IT_STBITERR);
		
		//~ if((sd_handle.Context & SD_CONTEXT_DMA) != RESET)
		//~ {
			//~ /* Abort the SD DMA Streams */
			//~ if(sd_handle.hdmatx != NULL)
			//~ {
				//~ /* Set the DMA Tx abort callback */
				//~ sd_handle.hdmatx->XferAbortCallback = SD_DMATxAbort;
				//~ /* Abort DMA in IT mode */
				//~ if(DMA_Abort_IT(sd_handle.hdmatx) !=0)
				//~ {
					//~ SD_DMATxAbort(sd_handle.hdmatx);
				//~ }
			//~ }
			//~ else if(sd_handle.hdmarx != NULL)
			//~ {
				//~ /* Set the DMA Rx abort callback */
				//~ sd_handle.hdmarx->XferAbortCallback = SD_DMARxAbort;
				//~ /* Abort DMA in IT mode */
				//~ if(DMA_Abort_IT(sd_handle.hdmarx) !=0)
				//~ {
					//~ SD_DMARxAbort(sd_handle.hdmarx);
				//~ }
			//~ }
			//~ else
			//~ {
				//~ sd_handle.ErrorCode = SD_ERROR_NONE;
				//~ sd_handle.State = SD_STATE_READY;
				//~ SD_AbortCallback(hsd);
			//~ }
		//~ }
		//~ else if((sd_handle.Context & SD_CONTEXT_IT) != RESET)
		//~ {
			//~ /* Set the SD state to ready to be able to start again the process */
			//~ sd_handle.State = SD_STATE_READY;
			//~ SD_ErrorCallback(hsd);
		//~ }
	//~ }
//~ #else /* SDIO_STA_STBITERR not defined */
	//~ else if(SD_GET_FLAG(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR) != RESET)
	//~ {
		//~ /* Set Error code */
		//~ if(SD_GET_FLAG(SDIO_IT_DCRCFAIL) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_DATA_CRC_FAIL; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_DTIMEOUT) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_DATA_TIMEOUT; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_RXOVERR) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_RX_OVERRUN; 
		//~ }
		//~ if(SD_GET_FLAG(SDIO_IT_TXUNDERR) != RESET)
		//~ {
			//~ sd_handle.ErrorCode |= SD_ERROR_TX_UNDERRUN; 
		//~ }

		//~ /* Clear All flags */
		//~ SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		
		//~ /* Disable all interrupts */
		//~ SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT|\
														 //~ SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR);
		
		//~ if((sd_handle.Context & SD_CONTEXT_DMA) != RESET)
		//~ {
			//~ /* Abort the SD DMA Streams */
			//~ if(sd_handle.hdmatx != NULL)
			//~ {
				//~ /* Set the DMA Tx abort callback */
				//~ sd_handle.hdmatx->XferAbortCallback = SD_DMATxAbort;
				//~ /* Abort DMA in IT mode */
				//~ if(DMA_Abort_IT(sd_handle.hdmatx) !=0)
				//~ {
					//~ SD_DMATxAbort(sd_handle.hdmatx);
				//~ }
			//~ }
			//~ else if(sd_handle.hdmarx != NULL)
			//~ {
				//~ /* Set the DMA Rx abort callback */
				//~ sd_handle.hdmarx->XferAbortCallback = SD_DMARxAbort;
				//~ /* Abort DMA in IT mode */
				//~ if(DMA_Abort_IT(sd_handle.hdmarx) !=0)
				//~ {
					//~ SD_DMARxAbort(sd_handle.hdmarx);
				//~ }
			//~ }
			//~ else
			//~ {
				//~ sd_handle.ErrorCode = SD_ERROR_NONE;
				//~ sd_handle.State = SD_STATE_READY;
				//~ SD_AbortCallback(hsd);
			//~ }
		//~ }
		//~ else if((sd_handle.Context & SD_CONTEXT_IT) != RESET)
		//~ {
			//~ /* Set the SD state to ready to be able to start again the process */
			//~ sd_handle.State = SD_STATE_READY;
			//~ SD_ErrorCallback(hsd);
		//~ }
	//~ }
//~ #endif
//~ }

/*==============================================================================
				##### Peripheral Control functions #####
 ==============================================================================*/
/* 
 * Returns information the information of the card which are stored on the CID register.
 */
uint32_t SD_GetCardCID(SD_CardCIDTypeDef *pCID)
{
	uint32_t tmp = 0U;
	
	/* Byte 0 */
	tmp = (uint8_t)((sd_handle.CID[0U] & 0xFF000000U) >> 24U);
	pCID->ManufacturerID = tmp;
	
	/* Byte 1 */
	tmp = (uint8_t)((sd_handle.CID[0U] & 0x00FF0000U) >> 16U);
	pCID->OEM_AppliID = tmp << 8U;
	
	/* Byte 2 */
	tmp = (uint8_t)((sd_handle.CID[0U] & 0x000000FF00U) >> 8U);
	pCID->OEM_AppliID |= tmp;
	
	/* Byte 3 */
	tmp = (uint8_t)(sd_handle.CID[0U] & 0x000000FFU);
	pCID->ProdName1 = tmp << 24U;
	
	/* Byte 4 */
	tmp = (uint8_t)((sd_handle.CID[1U] & 0xFF000000U) >> 24U);
	pCID->ProdName1 |= tmp << 16;
	
	/* Byte 5 */
	tmp = (uint8_t)((sd_handle.CID[1U] & 0x00FF0000U) >> 16U);
	pCID->ProdName1 |= tmp << 8U;
	
	/* Byte 6 */
	tmp = (uint8_t)((sd_handle.CID[1U] & 0x0000FF00U) >> 8U);
	pCID->ProdName1 |= tmp;
	
	/* Byte 7 */
	tmp = (uint8_t)(sd_handle.CID[1U] & 0x000000FFU);
	pCID->ProdName2 = tmp;
	
	/* Byte 8 */
	tmp = (uint8_t)((sd_handle.CID[2U] & 0xFF000000U) >> 24U);
	pCID->ProdRev = tmp;
	
	/* Byte 9 */
	tmp = (uint8_t)((sd_handle.CID[2U] & 0x00FF0000U) >> 16U);
	pCID->ProdSN = tmp << 24U;
	
	/* Byte 10 */
	tmp = (uint8_t)((sd_handle.CID[2U] & 0x0000FF00U) >> 8U);
	pCID->ProdSN |= tmp << 16U;
	
	/* Byte 11 */
	tmp = (uint8_t)(sd_handle.CID[2U] & 0x000000FFU);
	pCID->ProdSN |= tmp << 8U;
	
	/* Byte 12 */
	tmp = (uint8_t)((sd_handle.CID[3U] & 0xFF000000U) >> 24U);
	pCID->ProdSN |= tmp;
	
	/* Byte 13 */
	tmp = (uint8_t)((sd_handle.CID[3U] & 0x00FF0000U) >> 16U);
	pCID->Reserved1   |= (tmp & 0xF0U) >> 4U;
	pCID->ManufactDate = (tmp & 0x0FU) << 8U;
	
	/* Byte 14 */
	tmp = (uint8_t)((sd_handle.CID[3U] & 0x0000FF00U) >> 8U);
	pCID->ManufactDate |= tmp;
	
	/* Byte 15 */
	tmp = (uint8_t)(sd_handle.CID[3U] & 0x000000FFU);
	pCID->CID_CRC   = (tmp & 0xFEU) >> 1U;
	pCID->Reserved2 = 1U;

	return 0;
}

/*
 * Returns information the information of the card which are stored on the CSD register.
 */
uint32_t SD_GetCardCSD(SD_CardCSDTypeDef *pCSD)
{
	uint32_t tmp = 0U;
	
	/* Byte 0 */
	tmp = (sd_handle.CSD[0U] & 0xFF000000U) >> 24U;
	pCSD->CSDStruct      = (uint8_t)((tmp & 0xC0U) >> 6U);
	pCSD->SysSpecVersion = (uint8_t)((tmp & 0x3CU) >> 2U);
	pCSD->Reserved1      = tmp & 0x03U;
	/* Byte 1 */
	tmp = (sd_handle.CSD[0U] & 0x00FF0000U) >> 16U;
	pCSD->TAAC = (uint8_t)tmp;
	/* Byte 2 */
	tmp = (sd_handle.CSD[0U] & 0x0000FF00U) >> 8U;
	pCSD->NSAC = (uint8_t)tmp;
	/* Byte 3 */
	tmp = sd_handle.CSD[0U] & 0x000000FFU;
	pCSD->MaxBusClkFrec = (uint8_t)tmp;
	/* Byte 4 */
	tmp = (sd_handle.CSD[1U] & 0xFF000000U) >> 24U;
	pCSD->CardComdClasses = (uint16_t)(tmp << 4U);
	/* Byte 5 */
	tmp = (sd_handle.CSD[1U] & 0x00FF0000U) >> 16U;
	pCSD->CardComdClasses |= (uint16_t)((tmp & 0xF0U) >> 4U);
	pCSD->RdBlockLen       = (uint8_t)(tmp & 0x0FU);
	/* Byte 6 */
	tmp = (sd_handle.CSD[1U] & 0x0000FF00U) >> 8U;
	pCSD->PartBlockRead   = (uint8_t)((tmp & 0x80U) >> 7U);
	pCSD->WrBlockMisalign = (uint8_t)((tmp & 0x40U) >> 6U);
	pCSD->RdBlockMisalign = (uint8_t)((tmp & 0x20U) >> 5U);
	pCSD->DSRImpl         = (uint8_t)((tmp & 0x10U) >> 4U);
	pCSD->Reserved2       = 0U; /*!< Reserved */
			 
	if(sd_handle.SdCard.CardType == CARD_SDSC) {
		pCSD->DeviceSize = (tmp & 0x03U) << 10U;
		/* Byte 7 */
		tmp = (uint8_t)(sd_handle.CSD[1U] & 0x000000FFU);
		pCSD->DeviceSize |= (tmp) << 2U;
		/* Byte 8 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0xFF000000U) >> 24U);
		pCSD->DeviceSize |= (tmp & 0xC0U) >> 6U;
		pCSD->MaxRdCurrentVDDMin = (tmp & 0x38U) >> 3U;
		pCSD->MaxRdCurrentVDDMax = (tmp & 0x07U);
		/* Byte 9 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0x00FF0000U) >> 16U);
		pCSD->MaxWrCurrentVDDMin = (tmp & 0xE0U) >> 5U;
		pCSD->MaxWrCurrentVDDMax = (tmp & 0x1CU) >> 2U;
		pCSD->DeviceSizeMul      = (tmp & 0x03U) << 1U;
		/* Byte 10 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0x0000FF00U) >> 8U);
		pCSD->DeviceSizeMul |= (tmp & 0x80U) >> 7U;
		sd_handle.SdCard.BlockNbr  = (pCSD->DeviceSize + 1U) ;
		sd_handle.SdCard.BlockNbr *= (1U << (pCSD->DeviceSizeMul + 2U));
		sd_handle.SdCard.BlockSize = 1U << (pCSD->RdBlockLen);
		sd_handle.SdCard.LogBlockNbr =  (sd_handle.SdCard.BlockNbr) * ((sd_handle.SdCard.BlockSize) / 512U); 
		sd_handle.SdCard.LogBlockSize = 512U;
	} else if(sd_handle.SdCard.CardType == CARD_SDHC_SDXC) {
		/* Byte 7 */
		tmp = (uint8_t)(sd_handle.CSD[1U] & 0x000000FFU);
		pCSD->DeviceSize = (tmp & 0x3FU) << 16U;
		/* Byte 8 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0xFF000000U) >> 24U);
		pCSD->DeviceSize |= (tmp << 8U);
		/* Byte 9 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0x00FF0000U) >> 16U);
		pCSD->DeviceSize |= (tmp);
		/* Byte 10 */
		tmp = (uint8_t)((sd_handle.CSD[2U] & 0x0000FF00U) >> 8U);
		sd_handle.SdCard.LogBlockNbr = sd_handle.SdCard.BlockNbr = (((uint64_t)pCSD->DeviceSize + 1U) * 1024U);
		sd_handle.SdCard.LogBlockSize = sd_handle.SdCard.BlockSize = 512U;
	} else {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);   
		return SD_ERROR_UNSUPPORTED_FEATURE;
	}
	pCSD->EraseGrSize = (tmp & 0x40U) >> 6U;
	pCSD->EraseGrMul  = (tmp & 0x3FU) << 1U;
	/* Byte 11 */
	tmp = (uint8_t)(sd_handle.CSD[2U] & 0x000000FFU);
	pCSD->EraseGrMul     |= (tmp & 0x80U) >> 7U;
	pCSD->WrProtectGrSize = (tmp & 0x7FU);
	/* Byte 12 */
	tmp = (uint8_t)((sd_handle.CSD[3U] & 0xFF000000U) >> 24U);
	pCSD->WrProtectGrEnable = (tmp & 0x80U) >> 7U;
	pCSD->ManDeflECC        = (tmp & 0x60U) >> 5U;
	pCSD->WrSpeedFact       = (tmp & 0x1CU) >> 2U;
	pCSD->MaxWrBlockLen     = (tmp & 0x03U) << 2U;
	/* Byte 13 */
	tmp = (uint8_t)((sd_handle.CSD[3U] & 0x00FF0000U) >> 16U);
	pCSD->MaxWrBlockLen      |= (tmp & 0xC0U) >> 6U;
	pCSD->WriteBlockPaPartial = (tmp & 0x20U) >> 5U;
	pCSD->Reserved3           = 0U;
	pCSD->ContentProtectAppli = (tmp & 0x01U);
	/* Byte 14 */
	tmp = (uint8_t)((sd_handle.CSD[3U] & 0x0000FF00U) >> 8U);
	pCSD->FileFormatGrouop = (tmp & 0x80U) >> 7U;
	pCSD->CopyFlag         = (tmp & 0x40U) >> 6U;
	pCSD->PermWrProtect    = (tmp & 0x20U) >> 5U;
	pCSD->TempWrProtect    = (tmp & 0x10U) >> 4U;
	pCSD->FileFormat       = (tmp & 0x0CU) >> 2U;
	pCSD->ECC              = (tmp & 0x03U);
	/* Byte 15 */
	tmp = (uint8_t)(sd_handle.CSD[3U] & 0x000000FFU);
	pCSD->CSD_CRC   = (tmp & 0xFEU) >> 1U;
	pCSD->Reserved4 = 1U;
	
	return 0;
}

/*
 * Gets the SD status info.
 */
uint32_t SD_GetCardStatus(SD_CardStatusTypeDef *pStatus)
{
	uint32_t tmp = 0U;
	uint32_t sd_status[16U];
	uint32_t errorstate = SD_ERROR_NONE;
	
	errorstate = SD_SendSDStatus(sd_status);
	if(errorstate !=0) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);  
		return errorstate;
	} else {
		/* Byte 0 */
		tmp = (sd_status[0U] & 0xC0U) >> 6U;
		pStatus->DataBusWidth = (uint8_t)tmp;
		/* Byte 1 */
		tmp = (sd_status[0U] & 0x20U) >> 5U;
		pStatus->SecuredMode = (uint8_t)tmp;
		/* Byte 2 */
		tmp = (sd_status[0U] & 0x00FF0000U) >> 16U;
		pStatus->CardType = (uint16_t)(tmp << 8U);
		/* Byte 3 */
		tmp = (sd_status[0U] & 0xFF000000U) >> 24U;
		pStatus->CardType |= (uint16_t)tmp;
		/* Byte 4 */
		tmp = (sd_status[1U] & 0xFFU);
		pStatus->ProtectedAreaSize = (uint32_t)(tmp << 24U);
		/* Byte 5 */
		tmp = (sd_status[1U] & 0xFF00U) >> 8U;
		pStatus->ProtectedAreaSize |= (uint32_t)(tmp << 16U);
		/* Byte 6 */
		tmp = (sd_status[1U] & 0xFF0000U) >> 16U;
		pStatus->ProtectedAreaSize |= (uint32_t)(tmp << 8U);
		/* Byte 7 */
		tmp = (sd_status[1U] & 0xFF000000U) >> 24U;
		pStatus->ProtectedAreaSize |= (uint32_t)tmp;
		/* Byte 8 */
		tmp = (sd_status[2U] & 0xFFU);
		pStatus->SpeedClass = (uint8_t)tmp;
		/* Byte 9 */
		tmp = (sd_status[2U] & 0xFF00U) >> 8U;
		pStatus->PerformanceMove = (uint8_t)tmp;
		/* Byte 10 */
		tmp = (sd_status[2U] & 0xF00000U) >> 20U;
		pStatus->AllocationUnitSize = (uint8_t)tmp;
		/* Byte 11 */
		tmp = (sd_status[2U] & 0xFF000000U) >> 24U;
		pStatus->EraseSize = (uint16_t)(tmp << 8U);
		/* Byte 12 */
		tmp = (sd_status[3U] & 0xFFU);
		pStatus->EraseSize |= (uint16_t)tmp;
		/* Byte 13 */
		tmp = (sd_status[3U] & 0xFC00U) >> 10U;
		pStatus->EraseTimeout = (uint8_t)tmp;
		/* Byte 13 */
		tmp = (sd_status[3U] & 0x0300U) >> 8U;
		pStatus->EraseOffset = (uint8_t)tmp;
	}
	
	return 0;
}

/*
 * Gets the SD card info.
 */
int32_t SD_GetCardInfo(SD_CardInfoTypeDef *pCardInfo)
{
	pCardInfo->CardType     = (uint32_t)(sd_handle.SdCard.CardType);
	pCardInfo->CardVersion  = (uint32_t)(sd_handle.SdCard.CardVersion);
	pCardInfo->Class        = (uint32_t)(sd_handle.SdCard.Class);
	pCardInfo->RelCardAdd   = (uint32_t)(sd_handle.SdCard.RelCardAdd);
	pCardInfo->BlockNbr     = (uint32_t)(sd_handle.SdCard.BlockNbr);
	pCardInfo->BlockSize    = (uint32_t)(sd_handle.SdCard.BlockSize);
	pCardInfo->LogBlockNbr  = (uint32_t)(sd_handle.SdCard.LogBlockNbr);
	pCardInfo->LogBlockSize = (uint32_t)(sd_handle.SdCard.LogBlockSize);
	
	return 0;
}

/*
 * Enables wide bus operation for the requested card if supported 
 */
int32_t SD_ConfigWideBusOperation(uint32_t WideMode)
{
	//SDIO_InitTypeDef Init;
	uint32_t errorstate = 0;
	
	
	if(sd_handle.SdCard.CardType != CARD_SECURED)  {
		if(WideMode == SDIO_BUS_WIDE_8B) {
			errorstate |= SD_ERROR_UNSUPPORTED_FEATURE;
		} else if(WideMode == SDIO_BUS_WIDE_4B) {
			errorstate = SD_WideBus_Enable();
		} else if(WideMode == SDIO_BUS_WIDE_1B) {
			errorstate = SD_WideBus_Disable();
		} else {
			/* WideMode is not a valid argument*/
			errorstate |= SD_ERROR_PARAM;
		}
	} else {
		/* MMC Card does not support this feature */
		errorstate |= SD_ERROR_UNSUPPORTED_FEATURE;
	}
	
	if(errorstate != SD_ERROR_NONE) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		return errorstate;
	} else {
		SDIO_Init(SDIO, SDIO_BUS_WIDE_4B);
	}
	
	return 0;
}

/*
 * Gets the current sd card data state.
 */
//~ SD_CardStateTypeDef SD_GetCardState()
//~ {
	//~ SD_CardStateTypeDef cardstate =  SD_CARD_TRANSFER;
	//~ uint32_t errorstate = SD_ERROR_NONE;
	//~ uint32_t resp1 = 0;
	
	//~ errorstate = SD_SendStatus(sd_handle, &resp1);
	//~ if(errorstate !=0) {
		//~ sd_handle.ErrorCode |= errorstate;
	//~ }
	//~ cardstate = (SD_CardStateTypeDef)((resp1 >> 9U) & 0x0FU);
	
	//~ return cardstate;
//~ }

/*
 * Abort the current transfer and disable the SD.
 */
//~ int32_t SD_Abort()
//~ {
	//~ SD_CardStateTypeDef CardState;
	
	/* DIsable All interrupts */
	//~ SD_DISABLE_IT(sd_handle, SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT| SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR);
	/* Clear All flags */
	//~ SD_CLEAR_FLAG(sd_handle, SDIO_STATIC_FLAGS);
	/////////// FIXME /////////////////
	//~ if((sd_handle.hdmatx != NULL) || (sd_handle.hdmarx != NULL)) {
		//~ /* Disable the SD DMA request */
		//~ SDIO->DCTRL &= (uint32_t)~((uint32_t)SDIO_DCTRL_DMAEN);
		//~ /* Abort the SD DMA Tx Stream */
		//~ if(sd_handle.hdmatx != NULL) {
			//~ DMA_Abort(sd_handle.hdmatx);
		//~ }
		//~ /* Abort the SD DMA Rx Stream */
		//~ if(sd_handle.hdmarx != NULL) {
			//~ DMA_Abort(sd_handle.hdmarx);
		//~ }
	//~ }
	
	//~ sd_handle.State = SD_STATE_READY;
	//~ CardState = SD_GetCardState(hsd);
	//~ if((CardState == SD_CARD_RECEIVING) || (CardState == SD_CARD_SENDING)) {
		//~ sd_handle.ErrorCode = SDMMC_CmdStopTransfer(SDIO);
	//~ }
	//~ if(sd_handle.ErrorCode != SD_ERROR_NONE) {
		//~ return -1;
	//~ }
	//~ return 0;
//~ }
	
/* Private function ----------------------------------------------------------*/  
/*
 * DMA SD transmit process complete callback 
 */
//~ static void SD_DMATransmitCplt(DMA_HandleTypeDef *hdma)     
//~ {
	///// FIXME ///////
	//~ SD_HandleTypeDef* hsd = (SD_HandleTypeDef* )(hdma->Parent);
	//~ /* Enable DATAEND Interrupt */
	//~ SD_ENABLE_IT((SDIO_IT_DATAEND));
//~ }

/*
 * DMA SD receive process complete callback 
 */
//~ static void SD_DMAReceiveCplt(DMA_HandleTypeDef *hdma)  
//~ {
	/////////// FIXME ////////////
	//~ SD_HandleTypeDef* hsd = (SD_HandleTypeDef* )(hdma->Parent);
	//~ uint32_t errorstate = SD_ERROR_NONE;
	
	//~ /* Send stop command in multiblock write */
	//~ if(sd_handle.Context == (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA)) {
		//~ errorstate = SDMMC_CmdStopTransfer(SDIO);
		//~ if(errorstate != SD_ERROR_NONE) {
			//~ sd_handle.ErrorCode |= errorstate;
		//~ }
	//~ }
	
	//~ /* Disable the DMA transfer for transmit request by setting the DMAEN bit
	//~ in the SD DCTRL register */
	//~ SDIO->DCTRL &= (uint32_t)~((uint32_t)SDIO_DCTRL_DMAEN);
	//~ /* Clear all the static flags */
	//~ SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
	//~ sd_handle.State = SD_STATE_READY;
//~ }

/*
 * DMA SD communication error callback 
 */
//~ static void SD_DMAError(DMA_HandleTypeDef *hdma)   
//~ {
	//////////// FIXME ///////////////
	//~ SD_HandleTypeDef* hsd = (SD_HandleTypeDef* )(hdma->Parent);
	//~ SD_CardStateTypeDef CardState;
	
	//~ if((sd_handle.hdmarx->ErrorCode == DMA_ERROR_TE) || (sd_handle.hdmatx->ErrorCode == DMA_ERROR_TE)) {
		//~ /* Clear All flags */
		//~ SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		//~ /* Disable All interrupts */
		//~ SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT| SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR);
		//~ sd_handle.ErrorCode |= SD_ERROR_DMA;
		//~ CardState = SD_GetCardState(hsd);
		//~ if((CardState == SD_CARD_RECEIVING) || (CardState == SD_CARD_SENDING)) {
			//~ sd_handle.ErrorCode |= SDMMC_CmdStopTransfer(SDIO);
		//~ }
		//~ sd_handle.State= SD_STATE_READY;
	//~ }
//~ }

/*
 * DMA SD Tx Abort callback 
 */
//~ static void SD_DMATxAbort(DMA_HandleTypeDef *hdma)   
//~ {
	///////////// FIXME ////////////
	//~ SD_HandleTypeDef* hsd = (SD_HandleTypeDef* )(hdma->Parent);
	//~ SD_CardStateTypeDef CardState;
	
	//~ if(sd_handle.hdmatx != NULL) {
		//~ sd_handle.hdmatx = NULL;
	//~ }
	
	//~ /* All DMA channels are aborted */
	//~ if(sd_handle.hdmarx == NULL) {
		//~ CardState = SD_GetCardState(hsd);
		//~ sd_handle.ErrorCode = SD_ERROR_NONE;
		//~ sd_handle.State = SD_STATE_READY;
		//~ if((CardState == SD_CARD_RECEIVING) || (CardState == SD_CARD_SENDING)) {
			//~ sd_handle.ErrorCode |= SDMMC_CmdStopTransfer(SDIO);
			
			//~ if(sd_handle.ErrorCode != SD_ERROR_NONE) {
				//~ SD_AbortCallback(hsd);
			//~ } else {
				//~ SD_ErrorCallback(hsd);
			//~ }
		//~ }
	//~ }
//~ }

/*
 * DMA SD Rx Abort callback 
 */
//~ static void SD_DMARxAbort(DMA_HandleTypeDef *hdma)   
//~ {
	//////////// FIXME /////////////////
	//~ SD_HandleTypeDef* hsd = (SD_HandleTypeDef* )(hdma->Parent);
	//~ SD_CardStateTypeDef CardState;
	
	//~ if(sd_handle.hdmarx != NULL) {
		//~ sd_handle.hdmarx = NULL;
	//~ }
	
	//~ /* All DMA channels are aborted */
	//~ if(sd_handle.hdmatx == NULL) {
		//~ CardState = SD_GetCardState(hsd);
		//~ sd_handle.ErrorCode = SD_ERROR_NONE;
		//~ sd_handle.State = SD_STATE_READY;
		//~ if((CardState == SD_CARD_RECEIVING) || (CardState == SD_CARD_SENDING)) {
			//~ sd_handle.ErrorCode |= SDMMC_CmdStopTransfer(SDIO);
			
			//~ if(sd_handle.ErrorCode != SD_ERROR_NONE) {
				//~ SD_AbortCallback(hsd);
			//~ } else {
				//~ SD_ErrorCallback(hsd);
			//~ }
		//~ }
	//~ }
//~ }


/* 
 * Initializes the sd card.
 */
static uint32_t _SD_InitCard()
{
	SD_CardCSDTypeDef CSD;
	uint32_t errorstate = SD_ERROR_NONE;
	uint16_t sd_rca = 1U;
	
	/* Check the power State */
	if(SDIO_GetPowerState(SDIO) == 0U)  {
		/* Power off */
		return SD_ERROR_REQUEST_NOT_APPLICABLE;
	}
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Send CMD2 ALL_SEND_CID */
		errorstate = SDMMC_CmdSendCID(SDIO);
		if(errorstate != SD_ERROR_NONE) {
			return errorstate;
		} else {
			/* Get Card identification number data */
			sd_handle.CID[0U] = SDIO_GetResponse(SDIO, SDIO_RESP1);
			sd_handle.CID[1U] = SDIO_GetResponse(SDIO, SDIO_RESP2);
			sd_handle.CID[2U] = SDIO_GetResponse(SDIO, SDIO_RESP3);
			sd_handle.CID[3U] = SDIO_GetResponse(SDIO, SDIO_RESP4);
		}
	}
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Send CMD3 SET_REL_ADDR with argument 0 */
		/* SD Card publishes its RCA. */
		errorstate = SDMMC_CmdSetRelAdd(SDIO, &sd_rca);
		if(errorstate != SD_ERROR_NONE) {
			return errorstate;
		}
	}
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Get the SD card RCA */
		sd_handle.SdCard.RelCardAdd = sd_rca;
		/* Send CMD9 SEND_CSD with argument as card's RCA */
		errorstate = SDMMC_CmdSendCSD(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != SD_ERROR_NONE) {
			return errorstate;
		} else {
			/* Get Card Specific Data */
			sd_handle.CSD[0U] = SDIO_GetResponse(SDIO, SDIO_RESP1);
			sd_handle.CSD[1U] = SDIO_GetResponse(SDIO, SDIO_RESP2);
			sd_handle.CSD[2U] = SDIO_GetResponse(SDIO, SDIO_RESP3);
			sd_handle.CSD[3U] = SDIO_GetResponse(SDIO, SDIO_RESP4);
		}
	}
	/* Get the Card Class */
	sd_handle.SdCard.Class = (SDIO_GetResponse(SDIO, SDIO_RESP2) >> 20U);
	/* Get CSD parameters */
	SD_GetCardCSD(&CSD);
	/* Select the Card */
	errorstate = SDMMC_CmdSelDesel(SDIO, (uint32_t)(((uint32_t)sd_handle.SdCard.RelCardAdd) << 16U));
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* Configure SDIO peripheral interface */     
	// SDIO_Init(SDIO);

	/* All cards are initialized */
	return SD_ERROR_NONE;
}

/* 
 * Enquires cards about their operating voltage and configures clock controls and stores SD information that 
 * will be needed in future in the SD handle.
 */
static uint32_t SD_PowerON()
{
	volatile uint32_t count = 0U;
	uint32_t response = 0U, validvoltage = 0U;
	uint32_t errorstate = SD_ERROR_NONE;
	
	/* CMD0: GO_IDLE_STATE */
	errorstate = SDMMC_CmdGoIdleState(SDIO);
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* CMD8: SEND_IF_COND: Command available only on V2.0 cards */
	errorstate = SDMMC_CmdOperCond(SDIO);
	if(errorstate != SD_ERROR_NONE) {
		sd_handle.SdCard.CardVersion = CARD_V1_X;	
		/* Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
		while(validvoltage == 0U) {
			if(count++ == SDMMC_MAX_VOLT_TRIAL) {
				return SD_ERROR_INVALID_VOLTRANGE;
			}
			/* SEND CMD55 APP_CMD with RCA as 0 */
			errorstate = SDMMC_CmdAppCommand(SDIO, 0U);
			if(errorstate != SD_ERROR_NONE) {
				return SD_ERROR_UNSUPPORTED_FEATURE;
			}
			/* Send CMD41 */
			errorstate = SDMMC_CmdAppOperCommand(SDIO, SDMMC_STD_CAPACITY);
			if(errorstate != SD_ERROR_NONE) {
				return SD_ERROR_UNSUPPORTED_FEATURE;
			}
			/* Get command response */
			response = SDIO_GetResponse(SDIO, SDIO_RESP1);
			/* Get operating voltage*/
			validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);
		}
		/* Card type is SDSC */
		sd_handle.SdCard.CardType = CARD_SDSC;
	} else {
		sd_handle.SdCard.CardVersion = CARD_V2_X;		
		/* Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
		while(validvoltage == 0U) {
			if(count++ == SDMMC_MAX_VOLT_TRIAL) {
				return SD_ERROR_INVALID_VOLTRANGE;
			}
			/* SEND CMD55 APP_CMD with RCA as 0 */
			errorstate = SDMMC_CmdAppCommand(SDIO, 0U);
			if(errorstate != SD_ERROR_NONE) {
				return errorstate;
			}
			/* Send CMD41 */
			errorstate = SDMMC_CmdAppOperCommand(SDIO, SDMMC_HIGH_CAPACITY);
			if(errorstate != SD_ERROR_NONE) {
				return errorstate;
			}
			/* Get command response */
			response = SDIO_GetResponse(SDIO, SDIO_RESP1);
			/* Get operating voltage*/
			validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);
		}
		
		if((response & SDMMC_HIGH_CAPACITY) == SDMMC_HIGH_CAPACITY){
			sd_handle.SdCard.CardType = CARD_SDHC_SDXC;
		} else {
			sd_handle.SdCard.CardType = CARD_SDSC;
		}
	}
	
	return SD_ERROR_NONE;
}

/*
 * Turns the SDIO output signals off.
 */
static uint32_t SD_PowerOFF()
{
	/* Set Power State to OFF */
	SDIO_PowerState_OFF(SDIO);
	
	return 0;
}

/*
 * Send Status info command.
 */
uint32_t SD_SendSDStatus(uint32_t *pSDstatus)
{
	SDIO_DataInitTypeDef config;
	uint32_t errorstate = SD_ERROR_NONE;
	uint32_t tickstart = systick_get_tick_count();
	uint32_t count = 0U;
	
	/* Check SD response */
	if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED){
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Set block size for card if it is not equal to current block size for card */
	errorstate = SDMMC_CmdBlockLength(SDIO, 64U);
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* Send CMD55 */
	errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* Configure the SD DPSM (Data Path State Machine) */ 
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 64U;
	config.DataBlockSize = SDIO_DATABLOCK_SIZE_64B;
	config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
	config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDIO_DPSM_ENABLE;
	SDIO_ConfigData(SDIO, &config);
	/* Send ACMD13 (SD_APP_STAUS)  with argument as card's RCA */
	errorstate = SDMMC_CmdStatusRegister(SDIO);
	if(errorstate != SD_ERROR_NONE) {
		return errorstate;
	}
	/* Get status data */
	while(!SD_GET_FLAG(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND)) {
		if(SD_GET_FLAG(SDIO_FLAG_RXFIFOHF)) {
			for(count = 0U; count < 8U; count++) {
				*(pSDstatus + count) = SDIO_ReadFIFO(SDIO);
			}
			
			pSDstatus += 8U;
		}
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT)
		{
			return SD_ERROR_TIMEOUT;
		}
	}
	
	if(SD_GET_FLAG(SDIO_FLAG_DTIMEOUT)){
		return SD_ERROR_DATA_TIMEOUT;
	} else if(SD_GET_FLAG(SDIO_FLAG_DCRCFAIL)) {
		return SD_ERROR_DATA_CRC_FAIL;
	} else if(SD_GET_FLAG(SDIO_FLAG_RXOVERR)) {
		return SD_ERROR_RX_OVERRUN;
	}

	while ((SD_GET_FLAG(SDIO_FLAG_RXDAVL))) {
		*pSDstatus = SDIO_ReadFIFO(SDIO);
		pSDstatus++;
		
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT) {
			return SD_ERROR_TIMEOUT;
		}
	}
	
	/* Clear all the static status flags*/
	SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
	
	return SD_ERROR_NONE;
}

/* 
 * Returns the current card's status.
 */
//~ static uint32_t SD_SendStatus(uint32_t *pCardStatus)
//~ {
	//~ uint32_t errorstate = SD_ERROR_NONE;
	
	//~ if(pCardStatus == NULL) {
		//~ return SD_ERROR_PARAM;
	//~ }
	//~ /* Send Status command */
	//~ errorstate = SDMMC_CmdSendStatus(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
	//~ if(errorstate !=0) {
		//~ return errorstate;
	//~ }
	//~ /* Get SD card status */
	//~ *pCardStatus = SDIO_GetResponse(SDIO, SDIO_RESP1);
	
	//~ return SD_ERROR_NONE;
//~ }

/*
 * Enables the SDIO wide bus mode.
 */
static uint32_t SD_WideBus_Enable()
{
	uint32_t scr[2U] = {0U, 0U};
	uint32_t errorstate = SD_ERROR_NONE;
	
	if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Get SCR Register */
	errorstate = SD_FindSCR(scr);
	if(errorstate != 0) {
		return errorstate;
	}
	/* If requested card supports wide bus operation */
	if((scr[1U] & SDMMC_WIDE_BUS_SUPPORT) != SDMMC_ALLZERO) {
		/* Send CMD55 APP_CMD with argument as card's RCA.*/
		errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != 0) {
			return errorstate;
		}
		/* Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
		errorstate = SDMMC_CmdBusWidth(SDIO, 2U);
		if(errorstate != 0) {
			return errorstate;
		}

		return SD_ERROR_NONE;
	}
	else
	{
		return SD_ERROR_REQUEST_NOT_APPLICABLE;
	}
}

/*
 * Disables the SDIO wide bus mode.
 */
static uint32_t SD_WideBus_Disable()
{
	uint32_t scr[2U] = {0U, 0U};
	uint32_t errorstate = SD_ERROR_NONE;
	
	if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Get SCR Register */
	errorstate = SD_FindSCR(scr);
	if(errorstate != 0) {
		return errorstate;
	}
	/* If requested card supports 1 bit mode operation */
	if((scr[1U] & SDMMC_SINGLE_BUS_SUPPORT) != SDMMC_ALLZERO) {
		/* Send CMD55 APP_CMD with argument as card's RCA */
		errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != 0) {
			return errorstate;
		}
		/* Send ACMD6 APP_CMD with argument as 0 for single bus mode */
		errorstate = SDMMC_CmdBusWidth(SDIO, 0U);
		if(errorstate != 0) {
			return errorstate;
		}
		return SD_ERROR_NONE;
	} else {
		return SD_ERROR_REQUEST_NOT_APPLICABLE;
	}
}
	
/*
 *  Finds the SD card SCR register value.
 */
static uint32_t SD_FindSCR(uint32_t *pSCR)
{
	SDIO_DataInitTypeDef config;
	uint32_t errorstate = SD_ERROR_NONE;
	uint32_t tickstart = systick_get_tick_count();
	uint32_t index = 0U;
	uint32_t tempscr[2U] = {0U, 0U};
	
	/* Set Block Size To 8 Bytes */
	errorstate = SDMMC_CmdBlockLength(SDIO, 8U);
	if(errorstate != 0) {
		return errorstate;
	}
	/* Send CMD55 APP_CMD with argument as card's RCA */
	errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)((sd_handle.SdCard.RelCardAdd) << 16U));
	if(errorstate != 0) {
		return errorstate;
	}
	
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 8U;
	config.DataBlockSize = SDIO_DATABLOCK_SIZE_8B;
	config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
	config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDIO_DPSM_ENABLE;
	SDIO_ConfigData(SDIO, &config);
	/* Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
	errorstate = SDMMC_CmdSendSCR(SDIO);
	if(errorstate != 0) {
		return errorstate;
	}
	
	while(!SD_GET_FLAG(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND)) {
		if(SD_GET_FLAG(SDIO_FLAG_RXDAVL)) {
			*(tempscr + index) = SDIO_ReadFIFO(SDIO);
			index++;
		}
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT) {
			return SD_ERROR_TIMEOUT;
		}
	}
	
	if(SD_GET_FLAG(SDIO_FLAG_DTIMEOUT)) {
		SD_CLEAR_FLAG(SDIO_FLAG_DTIMEOUT);
		return SD_ERROR_DATA_TIMEOUT;
	} else if(SD_GET_FLAG(SDIO_FLAG_DCRCFAIL)) {
		SD_CLEAR_FLAG(SDIO_FLAG_DCRCFAIL);
		return SD_ERROR_DATA_CRC_FAIL;
	} else if(SD_GET_FLAG(SDIO_FLAG_RXOVERR)) {
		SD_CLEAR_FLAG(SDIO_FLAG_RXOVERR);
		return SD_ERROR_RX_OVERRUN;
	} else {
		/* No error flag set */
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		*(pSCR + 1U) = ((tempscr[0U] & SDMMC_0TO7BITS) << 24U)  | ((tempscr[0U] & SDMMC_8TO15BITS) << 8U) |\
						((tempscr[0U] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[0U] & SDMMC_24TO31BITS) >> 24U);
		*(pSCR) = ((tempscr[1U] & SDMMC_0TO7BITS) << 24U)  | ((tempscr[1U] & SDMMC_8TO15BITS) << 8U) |\
						((tempscr[1U] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[1U] & SDMMC_24TO31BITS) >> 24U);
	}

	return SD_ERROR_NONE;
}
