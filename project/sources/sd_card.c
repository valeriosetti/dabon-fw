#include "sd_card.h"
#include "sd_card_detect.h"
#include "systick.h"
#include "utils.h"
#include "debug_printf.h"

#define debug_msg(format, ...)		debug_printf("[sd] " format, ##__VA_ARGS__)

/* Private functions ---------------------------------------------------------*/
static uint32_t SD_PowerON(void);                      
static uint32_t _SD_InitCard(void);
static uint32_t SD_SendStatus(uint32_t *pCardStatus);
static uint32_t SD_WideBus_Enable(void);
static uint32_t SD_WideBus_Disable(void);
static uint32_t SD_FindSCR(uint32_t *pSCR);
//~ static uint32_t SD_PowerOFF(void);

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
	SDIO_Init(SDIO, SDIO_BUS_WIDE_1B, SDIO_INIT_CLK_DIV);
	/* Disable SDIO Clock */
	SD_DISABLE();
	/* Set Power State to ON */
	SDIO_PowerState_ON(SDIO);
	/* Enable SDIO Clock */
	SD_ENABLE();
	/* Required power up waiting time before starting the SD initialization  sequence */
	systick_wait_for_ms(2U);
	/* Identify card operating voltage */
	errorstate = SD_PowerON();
	if(errorstate != SD_ERROR_NONE) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__); 
		return errorstate;
	}
	/* Card initialization */
	errorstate = _SD_InitCard();
	if(errorstate != SD_ERROR_NONE) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	
	SD_ConfigWideBusOperation(SDIO_BUS_WIDE_4B);
	sd_handle.Context = SD_CONTEXT_NONE;

	return 0;
}

/*==============================================================================
				##### IO operation functions #####
==============================================================================  */
/*
 * Reads block(s) from a specified address in a card. The Data transfer is managed by DMA mode. 
 */
uint32_t SD_ReadBlocks_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks)
{		
	uint32_t errorstate = 0;
	if((BlockAdd + NumberOfBlocks) > (sd_handle.SdCard.LogBlockNbr)) {
		debug_msg("Error: SD_ERROR_ADDR_OUT_OF_RANGE in %s\n", __func__); 
		return SD_ERROR_ADDR_OUT_OF_RANGE;
	}
    
	/* Initialize data control register */
	SDIO->DCTRL = 0U;

	SD_ENABLE_IT((SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_DATAEND));

	/* Enable the DMA Channel */
	DMA2_Stream3->NDTR = (uint32_t)((BLOCKSIZE * NumberOfBlocks)/4);
	DMA2_Stream3->PAR = (uint32_t) &SDIO->FIFO;
	DMA2_Stream3->M0AR = (uint32_t) pData;
	// Enable the DMA
	SET_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);
	SD_DMA_ENABLE();

	if(sd_handle.SdCard.CardType != CARD_SDHC_SDXC) {
		BlockAdd *= 512U;
	}

	/* Configure the SD DPSM (Data Path State Machine) */ 
	SDIO_DataInitTypeDef config = {
		.DataTimeOut   = SDMMC_DATATIMEOUT,
		.DataLength    = BLOCKSIZE * NumberOfBlocks,
		.DataBlockSize = SDIO_DATABLOCK_SIZE_512B,
		.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO,
		.TransferMode  = SDIO_TRANSFER_MODE_BLOCK,
		.DPSM          = SDIO_DPSM_ENABLE,
	};	
	SDIO_ConfigData(SDIO, &config);
	/* Set Block Size for Card */ 
	errorstate = SDMMC_CmdBlockLength(SDIO, BLOCKSIZE);
	if(errorstate != SD_ERROR_NONE) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__); 
		return errorstate;
	}
	/* Read Blocks in DMA mode */
	if(NumberOfBlocks > 1U) {
		sd_handle.Context = (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA);
		/* Read Multi Block command */ 
		errorstate = SDMMC_CmdReadMultiBlock(SDIO, BlockAdd);
	} else {
		sd_handle.Context = (SD_CONTEXT_READ_SINGLE_BLOCK | SD_CONTEXT_DMA);
		/* Read Single Block command */ 
		errorstate = SDMMC_CmdReadSingleBlock(SDIO, BlockAdd);
	}
	if(errorstate != SD_ERROR_NONE) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__); 
		return errorstate;
	}

	return 0;
}

/*
 * Erases the specified memory area of the given SD card.
 */
uint32_t SD_Erase(uint32_t BlockStartAdd, uint32_t BlockEndAdd)
{
	uint32_t errorstate = SD_ERROR_NONE;
	
	if(BlockEndAdd < BlockStartAdd) {
		debug_msg("Error: SD_ERROR_PARAM in %s\n", __func__); 
		return SD_ERROR_PARAM;
	}
	
	if(BlockEndAdd > (sd_handle.SdCard.LogBlockNbr)) {
		debug_msg("Error: SD_ERROR_ADDR_OUT_OF_RANGE in %s\n", __func__); 
		return SD_ERROR_ADDR_OUT_OF_RANGE;
	}
	
	/* Check if the card command class supports erase command */
	if(((sd_handle.SdCard.Class) & SDIO_CCCC_ERASE) == 0U) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
		debug_msg("Error: SD_ERROR_REQUEST_NOT_APPLICABLE in %s\n", __func__); 
		return SD_ERROR_REQUEST_NOT_APPLICABLE;
	}
	if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);  
		debug_msg("Error: SD_ERROR_LOCK_UNLOCK_FAILED in %s\n", __func__); 
		return  SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Get start and end block for high capacity cards */
	if(sd_handle.SdCard.CardType != CARD_SDHC_SDXC) {
		BlockStartAdd *= 512U;
		BlockEndAdd   *= 512U;
	}
	/* According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and erase_group_end(CMD33) */
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Send CMD32 SD_ERASE_GRP_START with argument as addr  */
		errorstate = SDMMC_CmdSDEraseStartAdd(SDIO, BlockStartAdd);
		if(errorstate != SD_ERROR_NONE) {
			/* Clear all the static flags */
			SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__); 
			return errorstate;
		}
		/* Send CMD33 SD_ERASE_GRP_END with argument as addr  */
		errorstate = SDMMC_CmdSDEraseEndAdd(SDIO, BlockEndAdd);
		if(errorstate != SD_ERROR_NONE) {
			/* Clear all the static flags */
			SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__); 
			return errorstate;
		}
	}
	/* Send CMD38 ERASE */
	errorstate = SDMMC_CmdErase(SDIO);
	if(errorstate != SD_ERROR_NONE) {
		/* Clear all the static flags */
		SD_CLEAR_FLAG(SDIO_STATIC_FLAGS); 
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);  
		return errorstate;
	}
	
	return 0;
}

/*
 * Handle and clear SDIO's related DMA flags
 */
void SD_handle_and_clear_DMA_flags()
{
	// Print a message in case of DMA errors
	if (DMA2->LISR & DMA_LIFCR_CTEIF3_Msk) {
		debug_msg("Error: SDIO DMA transfer error\n");
	} else if (DMA2->LISR & DMA_LIFCR_CTEIF3_Msk) {
		debug_msg("Error: SDIO DMA direct mode error\n");
	} else if (DMA2->LISR & DMA_LIFCR_CFEIF3_Msk) {
		debug_msg("Error: SDIO DMA FIFO error\n");
	}
	// Clear all the interrupt flags
	DMA2->LIFCR = (DMA_LIFCR_CTCIF3_Msk | DMA_LIFCR_CHTIF3_Msk | DMA_LIFCR_CTEIF3_Msk |
					DMA_LIFCR_CDMEIF3_Msk | DMA_LIFCR_CFEIF3_Msk);
}

/*
 * Rx DMA interrupt handler 
 */
__attribute__((interrupt)) void DMA2_Stream3_IRQHandler()
{
	
}

/*
 * This function handles SD card interrupt request
 */
__attribute__((interrupt)) void SDIO_IRQHandler()
{
	uint32_t errorstate = SD_ERROR_NONE;
	
	/* Check for SDIO interrupt flags */
	if(SD_GET_FLAG(SDIO_IT_DATAEND) != 0) {
		if((sd_handle.Context & SD_CONTEXT_DMA) != 0)
		{
            if (sd_handle.Context & SD_CONTEXT_READ_MULTIPLE_BLOCK) {
                uint32_t errorstate = SDMMC_CmdStopTransfer(SDIO);
                if (errorstate != SDMMC_ERROR_NONE) {
                    debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
                }
            }
			SD_DMA_DISABLE();
			SD_handle_and_clear_DMA_flags();
			sd_handle.Context = SD_CONTEXT_NONE;
		}
	} else if(SD_GET_FLAG(SDIO_IT_TXFIFOHE) != 0) {
		debug_msg("Error: SDIO_IT_TXFIFOHE\n");
	} else if(SD_GET_FLAG(SDIO_IT_RXFIFOHF) != 0) {
		debug_msg("Error: SDIO_FLAG_RXFIFOHF\n");
	} else if(SD_GET_FLAG(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR) != 0) {
		if(SD_GET_FLAG(SDIO_IT_DCRCFAIL) != 0) {
			debug_msg("Error: SDIO_IT_DCRCFAIL\n");
		}
		if(SD_GET_FLAG(SDIO_IT_DTIMEOUT) != 0) {
			debug_msg("Error: SDIO_IT_DTIMEOUT\n");
		}
		if(SD_GET_FLAG(SDIO_IT_RXOVERR) != 0) {
			debug_msg("Error: SDIO_IT_RXOVERR\n");
		}
		if(SD_GET_FLAG(SDIO_IT_TXUNDERR) != 0) {
			debug_msg("Error: SDIO_IT_TXUNDERR\n");
		}
	}
	SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
	SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_TXUNDERR | SDIO_IT_RXOVERR);
}

/*
 * Return the current context of the peripheral
 */
uint32_t SD_GetContext()
{
	return sd_handle.Context;
}

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
		debug_msg("Error: SD_ERROR_UNSUPPORTED_FEATURE in %s\n", __func__);  
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
uint32_t SD_ConfigWideBusOperation(uint32_t WideMode)
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	} else {
		SDIO_Init(SDIO, SDIO_BUS_WIDE_4B, SDIO_TRANSFER_CLK_DIV);
	}
	
	return 0;
}

/*
 * Gets the current sd card data state.
 */
SD_CardStateTypeDef SD_GetCardState()
{
	SD_CardStateTypeDef cardstate =  SD_CARD_TRANSFER;
	uint32_t errorstate = SD_ERROR_NONE;
	uint32_t resp1 = 0;
	
	errorstate = SD_SendStatus(&resp1);
	cardstate = (SD_CardStateTypeDef)((resp1 >> 9U) & 0x0FU);
	
	return cardstate;
}

/*
 * Abort the current transfer and disable the SD.
 */
uint32_t SD_Abort()
{
	SD_CardStateTypeDef CardState;
	uint32_t error_code;
	
	//~ /* DIsable All interrupts */
	SD_DISABLE_IT(SDIO_IT_DATAEND | SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT| SDIO_IT_TXUNDERR| SDIO_IT_RXOVERR);
	//~ /* Clear All flags */
	SD_CLEAR_FLAG(SDIO_STATIC_FLAGS);
	/* Disable the SD DMA request */
	SDIO->DCTRL &= (uint32_t)~((uint32_t)SDIO_DCTRL_DMAEN);
	
	CardState = SD_GetCardState();
	if((CardState == SD_CARD_RECEIVING) || (CardState == SD_CARD_SENDING)) {
		error_code = SDMMC_CmdStopTransfer(SDIO);
	}
	if(error_code != SD_ERROR_NONE) {
		debug_msg("Error: error_code in %s\n", __func__);
		return error_code;
	}
	return 0;
}
	
/* Private function ----------------------------------------------------------*/  
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
		debug_msg("Error: SD_ERROR_REQUEST_NOT_APPLICABLE in %s\n", __func__);
		return SD_ERROR_REQUEST_NOT_APPLICABLE;
	}
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Send CMD2 ALL_SEND_CID */
		errorstate = SDMMC_CmdSendCID(SDIO);
		if(errorstate != SD_ERROR_NONE) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
			return errorstate;
		}
	}
	if(sd_handle.SdCard.CardType != CARD_SECURED) {
		/* Get the SD card RCA */
		sd_handle.SdCard.RelCardAdd = sd_rca;
		/* Send CMD9 SEND_CSD with argument as card's RCA */
		errorstate = SDMMC_CmdSendCSD(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != SD_ERROR_NONE) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* Configure SDIO peripheral interface */     
	SDIO_Init(SDIO, SDIO_BUS_WIDE_1B, SDIO_TRANSFER_CLK_DIV);

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
				debug_msg("Error: SD_ERROR_INVALID_VOLTRANGE in %s\n", __func__);
				return SD_ERROR_INVALID_VOLTRANGE;
			}
			/* SEND CMD55 APP_CMD with RCA as 0 */
			errorstate = SDMMC_CmdAppCommand(SDIO, 0U);
			if(errorstate != SD_ERROR_NONE) {
				debug_msg("Error: SD_ERROR_UNSUPPORTED_FEATURE in %s\n", __func__);
				return SD_ERROR_UNSUPPORTED_FEATURE;
			}
			/* Send CMD41 */
			errorstate = SDMMC_CmdAppOperCommand(SDIO, SDMMC_STD_CAPACITY);
			if(errorstate != SD_ERROR_NONE) {
				debug_msg("Error: SD_ERROR_UNSUPPORTED_FEATURE in %s\n", __func__);
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
				debug_msg("Error: SD_ERROR_INVALID_VOLTRANGE in %s\n", __func__);
				return SD_ERROR_INVALID_VOLTRANGE;
			}
			/* SEND CMD55 APP_CMD with RCA as 0 */
			errorstate = SDMMC_CmdAppCommand(SDIO, 0U);
			if(errorstate != SD_ERROR_NONE) {
				debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
				return errorstate;
			}
			/* Send CMD41 */
			errorstate = SDMMC_CmdAppOperCommand(SDIO, SDMMC_HIGH_CAPACITY);
			if(errorstate != SD_ERROR_NONE) {
				debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
		debug_msg("Error: SD_ERROR_LOCK_UNLOCK_FAILED in %s\n", __func__);
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Set block size for card if it is not equal to current block size for card */
	errorstate = SDMMC_CmdBlockLength(SDIO, 64U);
	if(errorstate != SD_ERROR_NONE) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* Send CMD55 */
	errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
	if(errorstate != SD_ERROR_NONE) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT) {
			debug_msg("Error: SD_ERROR_TIMEOUT in %s\n", __func__);
			return SD_ERROR_TIMEOUT;
		}
	}
	
	if(SD_GET_FLAG(SDIO_FLAG_DTIMEOUT)){
		debug_msg("Error: SD_ERROR_DATA_TIMEOUT in %s\n", __func__);
		return SD_ERROR_DATA_TIMEOUT;
	} else if(SD_GET_FLAG(SDIO_FLAG_DCRCFAIL)) {
		debug_msg("Error: SD_ERROR_DATA_CRC_FAIL in %s\n", __func__);
		return SD_ERROR_DATA_CRC_FAIL;
	} else if(SD_GET_FLAG(SDIO_FLAG_RXOVERR)) {
		debug_msg("Error: SD_ERROR_RX_OVERRUN in %s\n", __func__);
		return SD_ERROR_RX_OVERRUN;
	}

	while ((SD_GET_FLAG(SDIO_FLAG_RXDAVL))) {
		*pSDstatus = SDIO_ReadFIFO(SDIO);
		pSDstatus++;
		
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT) {
			debug_msg("Error: SD_ERROR_TIMEOUT in %s\n", __func__);
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
static uint32_t SD_SendStatus(uint32_t *pCardStatus)
{
	uint32_t errorstate = SD_ERROR_NONE;
	
	/* Send Status command */
	errorstate = SDMMC_CmdSendStatus(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
	if(errorstate !=0) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* Get SD card status */
	*pCardStatus = SDIO_GetResponse(SDIO, SDIO_RESP1);
	
	return SD_ERROR_NONE;
}

/*
 * Enables the SDIO wide bus mode.
 */
static uint32_t SD_WideBus_Enable()
{
	uint32_t scr[2U] = {0U, 0U};
	uint32_t errorstate = SD_ERROR_NONE;
	
	if((SDIO_GetResponse(SDIO, SDIO_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		debug_msg("Error: SD_ERROR_LOCK_UNLOCK_FAILED in %s\n", __func__);
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Get SCR Register */
	errorstate = SD_FindSCR(scr);
	if(errorstate != 0) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* If requested card supports wide bus operation */
	if((scr[1U] & SDMMC_WIDE_BUS_SUPPORT) != SDMMC_ALLZERO) {
		/* Send CMD55 APP_CMD with argument as card's RCA.*/
		errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != 0) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
			return errorstate;
		}
		/* Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
		errorstate = SDMMC_CmdBusWidth(SDIO, 2U);
		if(errorstate != 0) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
			return errorstate;
		}

		return SD_ERROR_NONE;
	} else {
		debug_msg("Error: SD_ERROR_REQUEST_NOT_APPLICABLE in %s\n", __func__);
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
		debug_msg("Error: SD_ERROR_LOCK_UNLOCK_FAILED in %s\n", __func__);
		return SD_ERROR_LOCK_UNLOCK_FAILED;
	}
	/* Get SCR Register */
	errorstate = SD_FindSCR(scr);
	if(errorstate != 0) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* If requested card supports 1 bit mode operation */
	if((scr[1U] & SDMMC_SINGLE_BUS_SUPPORT) != SDMMC_ALLZERO) {
		/* Send CMD55 APP_CMD with argument as card's RCA */
		errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)(sd_handle.SdCard.RelCardAdd << 16U));
		if(errorstate != 0) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
			return errorstate;
		}
		/* Send ACMD6 APP_CMD with argument as 0 for single bus mode */
		errorstate = SDMMC_CmdBusWidth(SDIO, 0U);
		if(errorstate != 0) {
			debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
			return errorstate;
		}
		return SD_ERROR_NONE;
	} else {
		debug_msg("Error: SD_ERROR_REQUEST_NOT_APPLICABLE in %s\n", __func__);
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	/* Send CMD55 APP_CMD with argument as card's RCA */
	errorstate = SDMMC_CmdAppCommand(SDIO, (uint32_t)((sd_handle.SdCard.RelCardAdd) << 16U));
	if(errorstate != 0) {
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
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
		debug_msg("Error: 0x%x in %s\n", errorstate, __func__);
		return errorstate;
	}
	
	while(!SD_GET_FLAG(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND)) {
		if(SD_GET_FLAG(SDIO_FLAG_RXDAVL)) {
			*(tempscr + index) = SDIO_ReadFIFO(SDIO);
			index++;
		}
		if((systick_get_tick_count() - tickstart) >=  SDMMC_DATATIMEOUT) {
			debug_msg("Error: SD_ERROR_TIMEOUT in %s\n", __func__);
			return SD_ERROR_TIMEOUT;
		}
	}
	
	if(SD_GET_FLAG(SDIO_FLAG_DTIMEOUT)) {
		SD_CLEAR_FLAG(SDIO_FLAG_DTIMEOUT);
		debug_msg("Error: SD_ERROR_DATA_TIMEOUT in %s\n", __func__);
		return SD_ERROR_DATA_TIMEOUT;
	} else if(SD_GET_FLAG(SDIO_FLAG_DCRCFAIL)) {
		SD_CLEAR_FLAG(SDIO_FLAG_DCRCFAIL);
		debug_msg("Error: SD_ERROR_DATA_CRC_FAIL in %s\n", __func__);
		return SD_ERROR_DATA_CRC_FAIL;
	} else if(SD_GET_FLAG(SDIO_FLAG_RXOVERR)) {
		SD_CLEAR_FLAG(SDIO_FLAG_RXOVERR);
		debug_msg("Error: SD_ERROR_RX_OVERRUN in %s\n", __func__);
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
