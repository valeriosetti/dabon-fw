#include "sd_card_ll.h"
#include "utils.h"

static uint32_t SDMMC_GetCmdError(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp1(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint32_t Timeout);
static uint32_t SDMMC_GetCmdResp2(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp3(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp7(SDIO_TypeDef *SDIOx);
static uint32_t SDMMC_GetCmdResp6(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint16_t *pRCA);

uint32_t SystemCoreClock;

/**
	* @brief  Initializes the SDMMC according to the specified
	*         parameters in the SDMMC_InitTypeDef and create the associated handle.
	* @param  SDIOx: Pointer to SDMMC register base
	* @param  Init: SDMMC initialization structure   
	* @retval HAL status
	*/
uint32_t SDIO_Init(SDIO_TypeDef *SDIOx, SDIO_InitTypeDef Init)
{
	uint32_t tmpreg = 0U;
	
	/* Set SDMMC configuration parameters */
	tmpreg |= (Init.ClockEdge           |\
						 Init.ClockBypass         |\
						 Init.ClockPowerSave      |\
						 Init.BusWide             |\
						 Init.HardwareFlowControl |\
						 Init.ClockDiv
						 ); 
	
	/* Write to SDMMC CLKCR */
	MODIFY_REG(SDIOx->CLKCR, CLKCR_CLEAR_MASK, tmpreg);  

	return 0;
}
	
/*===============================================================================
											##### I/O operation functions #####
 ===============================================================================  */
/**
	* @brief  Read data (word) from Rx FIFO in blocking mode (polling) 
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval HAL status
	*/
uint32_t SDIO_ReadFIFO(SDIO_TypeDef *SDIOx)
{
	/* Read data from Rx FIFO */ 
	return (SDIOx->FIFO);
}

/**
	* @brief  Write data (word) to Tx FIFO in blocking mode (polling) 
	* @param  SDIOx: Pointer to SDMMC register base
	* @param  pWriteData: pointer to data to write
	* @retval HAL status
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
	* @brief  Set SDMMC Power state to ON. 
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval HAL status
	*/
uint32_t SDIO_PowerState_ON(SDIO_TypeDef *SDIOx)
{  
	/* Set power state to ON */ 
	SDIOx->POWER = SDIO_POWER_PWRCTRL;
	
	return 0;
}

/**
	* @brief  Set SDMMC Power state to OFF. 
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval HAL status
	*/
uint32_t SDIO_PowerState_OFF(SDIO_TypeDef *SDIOx)
{
	/* Set power state to OFF */
	SDIOx->POWER = 0x00000000U;
	
	return 0;
}

/**
	* @brief  Get SDMMC Power state. 
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval Power status of the controller. The returned value can be one of the 
	*         following values:
	*            - 0x00: Power OFF
	*            - 0x02: Power UP
	*            - 0x03: Power ON 
	*/
uint32_t SDIO_GetPowerState(SDIO_TypeDef *SDIOx)  
{
	return (SDIOx->POWER & SDIO_POWER_PWRCTRL);
}

/**
	* @brief  Configure the SDMMC command path according to the specified parameters in
	*         SDIO_CmdInitTypeDef structure and send the command 
	* @param  SDIOx: Pointer to SDMMC register base
	* @param  Command: pointer to a SDIO_CmdInitTypeDef structure that contains 
	*         the configuration information for the SDMMC command
	* @retval HAL status
	*/
uint32_t SDIO_SendCommand(SDIO_TypeDef *SDIOx, SDIO_CmdInitTypeDef *Command)
{
	uint32_t tmpreg = 0U;

	/* Set the SDMMC Argument value */
	SDIOx->ARG = Command->Argument;

	/* Set SDMMC command parameters */
	tmpreg |= (uint32_t)(Command->CmdIndex         |\
											 Command->Response         |\
											 Command->WaitForInterrupt |\
											 Command->CPSM);
	
	/* Write to SDMMC CMD register */
	MODIFY_REG(SDIOx->CMD, CMD_CLEAR_MASK, tmpreg); 
	
	return 0;  
}

/**
	* @brief  Return the command index of last command for which response received
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval Command index of the last command response received
	*/
uint8_t SDIO_GetCommandResponse(SDIO_TypeDef *SDIOx)
{
	return (uint8_t)(SDIOx->RESPCMD);
}


/**
	* @brief  Return the response received from the card for the last command
	* @param  SDIOx: Pointer to SDMMC register base    
	* @param  Response: Specifies the SDMMC response register. 
	*          This parameter can be one of the following values:
	*            @arg SDIO_RESP1: Response Register 1
	*            @arg SDIO_RESP1: Response Register 2
	*            @arg SDIO_RESP1: Response Register 3
	*            @arg SDIO_RESP1: Response Register 4  
	* @retval The Corresponding response register value
	*/
uint32_t SDIO_GetResponse(SDIO_TypeDef *SDIOx, uint32_t Response)
{
	volatile uint32_t tmp = 0U;
	
	/* Get the response */
	tmp = (uint32_t)&(SDIOx->RESP1) + Response;
	
	return (*(volatile uint32_t *) tmp);
}  

/**
	* @brief  Configure the SDMMC data path according to the specified 
	*         parameters in the SDIO_DataInitTypeDef.
	* @param  SDIOx: Pointer to SDMMC register base  
	* @param  Data : pointer to a SDIO_DataInitTypeDef structure 
	*         that contains the configuration information for the SDMMC data.
	* @retval HAL status
	*/
uint32_t SDIO_ConfigData(SDIO_TypeDef *SDIOx, SDIO_DataInitTypeDef* Data)
{
	uint32_t tmpreg = 0U;

	/* Set the SDMMC Data TimeOut value */
	SDIOx->DTIMER = Data->DataTimeOut;

	/* Set the SDMMC DataLength value */
	SDIOx->DLEN = Data->DataLength;

	/* Set the SDMMC data configuration parameters */
	tmpreg |= (uint32_t)(Data->DataBlockSize |\
											 Data->TransferDir   |\
											 Data->TransferMode  |\
											 Data->DPSM);
	
	/* Write to SDMMC DCTRL */
	MODIFY_REG(SDIOx->DCTRL, DCTRL_CLEAR_MASK, tmpreg);

	return 0;

}

/**
	* @brief  Returns number of remaining data bytes to be transferred.
	* @param  SDIOx: Pointer to SDMMC register base
	* @retval Number of remaining data bytes to be transferred
	*/
uint32_t SDIO_GetDataCounter(SDIO_TypeDef *SDIOx)
{
	return (SDIOx->DCOUNT);
}

/**
	* @brief  Get the FIFO data
	* @param  SDIOx: Pointer to SDMMC register base 
	* @retval Data received
	*/
uint32_t SDIO_GetFIFOCount(SDIO_TypeDef *SDIOx)
{
	return (SDIOx->FIFO);
}

/**
	* @brief  Sets one of the two options of inserting read wait interval.
	* @param  SDIOx: Pointer to SDMMC register base   
	* @param  SDIO_ReadWaitMode: SDMMC Read Wait operation mode.
	*          This parameter can be:
	*            @arg SDIO_READ_WAIT_MODE_CLK: Read Wait control by stopping SDMMCCLK
	*            @arg SDIO_READ_WAIT_MODE_DATA2: Read Wait control using SDMMC_DATA2
	* @retval None
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
	* @brief  Send the Data Block Lenght command and check the response
	* @param  SDIOx: Pointer to SDMMC register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Read Single Block command and check the response
	* @param  SDIOx: Pointer to SDMMC register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Read Multi Block command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Write Single Block command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Write Multi Block command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Start Address Erase command for SD and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the End Address Erase command for SD and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Start Address Erase command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the End Address Erase command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Erase command and check the response
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Stop Transfer command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Select Deselect command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @param  addr: Address of the card to be selected  
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Go Idle State command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Operating Condition command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Application command to verify that that the next command 
	*         is an application specific com-mand rather than a standard command
	*         and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the command asking the accessed card to send its operating 
	*         condition register (OCR)
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Bus Width command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Send SCR command and check the response.
	* @param  SDIOx: Pointer to SDMMC register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Send CID command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Send CSD command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Send CSD command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Status command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Send the Status register command and check the response.
	* @param  SDIOx: Pointer to SDIO register base 
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Sends host capacity support information and activates the card's 
	*         initialization process. Send SDMMC_CMD_SEND_OP_COND command
	* @param  SDIOx: Pointer to SDIO register base 
	* @parame Argument: Argument used for the command
	* @retval HAL status
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

	return errorstate;
}

/**
	* @brief  Checks switchable function and switch card function. SDMMC_CMD_HS_SWITCH comand
	* @param  SDIOx: Pointer to SDIO register base 
	* @parame Argument: Argument used for the command
	* @retval HAL status
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

	return errorstate;
}

/* Private function ----------------------------------------------------------*/  
/**
	* @brief  Checks for error conditions for CMD0.
	* @param  hsd: SD handle
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdError(SDIO_TypeDef *SDIOx)
{
	/* 8 is the number of required instructions cycles for the below loop statement.
	The SDMMC_CMDTIMEOUT is expressed in ms */
	register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CMDSENT));
	
	/* Clear all the static flags */
	__SDIO_CLEAR_FLAG(SDIOx, SDIO_STATIC_FLAGS);
	
	return SDMMC_ERROR_NONE;
}

/**
	* @brief  Checks for error conditions for R1 response.
	* @param  hsd: SD handle
	* @param  SD_CMD: The sent command index  
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdResp1(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint32_t Timeout)
{
	uint32_t response_r1;
	
	/* 8 is the number of required instructions cycles for the below loop statement.
	The Timeout is expressed in ms */
	register uint32_t count = Timeout * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Check response received is of desired command */
	if(SDIO_GetCommandResponse(SDIOx) != SD_CMD)
	{
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
		return SDMMC_ERROR_ADDR_OUT_OF_RANGE;
	}
	else if((response_r1 & SDMMC_OCR_ADDR_MISALIGNED) == SDMMC_OCR_ADDR_MISALIGNED)
	{
		return SDMMC_ERROR_ADDR_MISALIGNED;
	}
	else if((response_r1 & SDMMC_OCR_BLOCK_LEN_ERR) == SDMMC_OCR_BLOCK_LEN_ERR)
	{
		return SDMMC_ERROR_BLOCK_LEN_ERR;
	}
	else if((response_r1 & SDMMC_OCR_ERASE_SEQ_ERR) == SDMMC_OCR_ERASE_SEQ_ERR)
	{
		return SDMMC_ERROR_ERASE_SEQ_ERR;
	}
	else if((response_r1 & SDMMC_OCR_BAD_ERASE_PARAM) == SDMMC_OCR_BAD_ERASE_PARAM)
	{
		return SDMMC_ERROR_BAD_ERASE_PARAM;
	}
	else if((response_r1 & SDMMC_OCR_WRITE_PROT_VIOLATION) == SDMMC_OCR_WRITE_PROT_VIOLATION)
	{
		return SDMMC_ERROR_WRITE_PROT_VIOLATION;
	}
	else if((response_r1 & SDMMC_OCR_LOCK_UNLOCK_FAILED) == SDMMC_OCR_LOCK_UNLOCK_FAILED)
	{
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_COM_CRC_FAILED) == SDMMC_OCR_COM_CRC_FAILED)
	{
		return SDMMC_ERROR_COM_CRC_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_ILLEGAL_CMD) == SDMMC_OCR_ILLEGAL_CMD)
	{
		return SDMMC_ERROR_ILLEGAL_CMD;
	}
	else if((response_r1 & SDMMC_OCR_CARD_ECC_FAILED) == SDMMC_OCR_CARD_ECC_FAILED)
	{
		return SDMMC_ERROR_CARD_ECC_FAILED;
	}
	else if((response_r1 & SDMMC_OCR_CC_ERROR) == SDMMC_OCR_CC_ERROR)
	{
		return SDMMC_ERROR_CC_ERR;
	}
	else if((response_r1 & SDMMC_OCR_STREAM_READ_UNDERRUN) == SDMMC_OCR_STREAM_READ_UNDERRUN)
	{
		return SDMMC_ERROR_STREAM_READ_UNDERRUN;
	}
	else if((response_r1 & SDMMC_OCR_STREAM_WRITE_OVERRUN) == SDMMC_OCR_STREAM_WRITE_OVERRUN)
	{
		return SDMMC_ERROR_STREAM_WRITE_OVERRUN;
	}
	else if((response_r1 & SDMMC_OCR_CID_CSD_OVERWRITE) == SDMMC_OCR_CID_CSD_OVERWRITE)
	{
		return SDMMC_ERROR_CID_CSD_OVERWRITE;
	}
	else if((response_r1 & SDMMC_OCR_WP_ERASE_SKIP) == SDMMC_OCR_WP_ERASE_SKIP)
	{
		return SDMMC_ERROR_WP_ERASE_SKIP;
	}
	else if((response_r1 & SDMMC_OCR_CARD_ECC_DISABLED) == SDMMC_OCR_CARD_ECC_DISABLED)
	{
		return SDMMC_ERROR_CARD_ECC_DISABLED;
	}
	else if((response_r1 & SDMMC_OCR_ERASE_RESET) == SDMMC_OCR_ERASE_RESET)
	{
		return SDMMC_ERROR_ERASE_RESET;
	}
	else if((response_r1 & SDMMC_OCR_AKE_SEQ_ERROR) == SDMMC_OCR_AKE_SEQ_ERROR)
	{
		return SDMMC_ERROR_AKE_SEQ_ERR;
	}
	else
	{
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}

/**
	* @brief  Checks for error conditions for R2 (CID or CSD) response.
	* @param  hsd: SD handle
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdResp2(SDIO_TypeDef *SDIOx)
{
	/* 8 is the number of required instructions cycles for the below loop statement.
	The SDMMC_CMDTIMEOUT is expressed in ms */
	register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
		
	if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if (__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		
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
	* @brief  Checks for error conditions for R3 (OCR) response.
	* @param  hsd: SD handle
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdResp3(SDIO_TypeDef *SDIOx)
{
	/* 8 is the number of required instructions cycles for the below loop statement.
	The SDMMC_CMDTIMEOUT is expressed in ms */
	register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		
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
	* @brief  Checks for error conditions for R6 (RCA) response.
	* @param  hsd: SD handle
	* @param  SD_CMD: The sent command index
	* @param  pRCA: Pointer to the variable that will contain the SD card relative 
	*         address RCA   
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdResp6(SDIO_TypeDef *SDIOx, uint8_t SD_CMD, uint16_t *pRCA)
{
	uint32_t response_r1;

	/* 8 is the number of required instructions cycles for the below loop statement.
	The SDMMC_CMDTIMEOUT is expressed in ms */
	register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT);
		
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	else if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL))
	{
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL);
		
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}
	
	/* Check response received is of desired command */
	if(SDIO_GetCommandResponse(SDIOx) != SD_CMD)
	{
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
		return SDMMC_ERROR_ILLEGAL_CMD;
	}
	else if((response_r1 & SDMMC_R6_COM_CRC_FAILED) == SDMMC_R6_COM_CRC_FAILED)
	{
		return SDMMC_ERROR_COM_CRC_FAILED;
	}
	else
	{
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}

/**
	* @brief  Checks for error conditions for R7 response.
	* @param  hsd: SD handle
	* @retval SD Card error state
	*/
static uint32_t SDMMC_GetCmdResp7(SDIO_TypeDef *SDIOx)
{
	/* 8 is the number of required instructions cycles for the below loop statement.
	The SDIO_CMDTIMEOUT is expressed in ms */
	register uint32_t count = SDIO_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
	
	do
	{
		if (count-- == 0U)
		{
			return SDMMC_ERROR_TIMEOUT;
		}
		
	}while(!__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT));

	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CTIMEOUT))
	{
		/* Card is SD V2.0 compliant */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CMDREND);
		
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	}
	
	if(__SDIO_GET_FLAG(SDIOx, SDIO_FLAG_CMDREND))
	{
		/* Card is SD V2.0 compliant */
		__SDIO_CLEAR_FLAG(SDIOx, SDIO_FLAG_CMDREND);
	}
	
	return SDMMC_ERROR_NONE;
	
}
