#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include "stdint.h"
#include "sdio.h"
/* 
 * SD State enumeration structure 
 */   
typedef enum
{
	SD_STATE_RESET                  = 0x00000000U,  /*!< SD not yet initialized or disabled  */
	SD_STATE_READY                  = 0x00000001U,  /*!< SD initialized and ready for use    */
	SD_STATE_TIMEOUT                = 0x00000002U,  /*!< SD Timeout state                    */
	SD_STATE_BUSY                   = 0x00000003U,  /*!< SD process ongoing                  */
	SD_STATE_PROGRAMMING            = 0x00000004U,  /*!< SD Programming State                */
	SD_STATE_RECEIVING              = 0x00000005U,  /*!< SD Receinving State                 */
	SD_STATE_TRANSFER               = 0x00000006U,  /*!< SD Transfert State                  */
	SD_STATE_ERROR                  = 0x0000000FU   /*!< SD is in error state                */
}SD_StateTypeDef;

/* 
 * SD Card State enumeration structure 
 */   
typedef enum
{
	SD_CARD_READY                  = 0x00000001U,  /*!< Card state is ready                     */
	SD_CARD_IDENTIFICATION         = 0x00000002U,  /*!< Card is in identification state         */
	SD_CARD_STANDBY                = 0x00000003U,  /*!< Card is in standby state                */
	SD_CARD_TRANSFER               = 0x00000004U,  /*!< Card is in transfer state               */  
	SD_CARD_SENDING                = 0x00000005U,  /*!< Card is sending an operation            */
	SD_CARD_RECEIVING              = 0x00000006U,  /*!< Card is receiving operation information */
	SD_CARD_PROGRAMMING            = 0x00000007U,  /*!< Card is in programming state            */
	SD_CARD_DISCONNECTED           = 0x00000008U,  /*!< Card is disconnected                    */
	SD_CARD_ERROR                  = 0x000000FFU   /*!< Card response Error                     */
}SD_CardStateTypeDef;

/* 
 * SD Card Information Structure definition
 */ 
typedef struct
{
	uint32_t CardType;                     /*!< Specifies the card Type                         */
	uint32_t CardVersion;                  /*!< Specifies the card version                      */
	uint32_t Class;                        /*!< Specifies the class of the card class           */
	uint32_t RelCardAdd;                   /*!< Specifies the Relative Card Address             */
	uint32_t BlockNbr;                     /*!< Specifies the Card Capacity in blocks           */
	uint32_t BlockSize;                    /*!< Specifies one block size in bytes               */
	uint32_t LogBlockNbr;                  /*!< Specifies the Card logical Capacity in blocks   */
	uint32_t LogBlockSize;                 /*!< Specifies logical block size in bytes           */
}SD_CardInfoTypeDef;

/* 
 * SD handle Structure definition
 */ 
typedef struct
{
	uint32_t                     *pTxBuffPtr;      /*!< Pointer to SD Tx transfer Buffer    */
	uint32_t                     TxXferSize;       /*!< SD Tx Transfer size                 */
	uint32_t                     *pRxBuffPtr;      /*!< Pointer to SD Rx transfer Buffer    */
	uint32_t                     RxXferSize;       /*!< SD Rx Transfer size                 */
	volatile uint32_t                Context;          /*!< SD transfer context                 */
	SD_CardInfoTypeDef       SdCard;           /*!< SD Card information                 */
	uint32_t                     CSD[4];           /*!< SD card specific data table         */
	uint32_t                     CID[4];           /*!< SD card identification number table */
}SD_HandleTypeDef;

/* 
 * Card Specific Data: CSD Register 
 */
typedef struct
{
	volatile uint8_t  CSDStruct;            /*!< CSD structure                         */
	volatile uint8_t  SysSpecVersion;       /*!< System specification version          */
	volatile uint8_t  Reserved1;            /*!< Reserved                              */
	volatile uint8_t  TAAC;                 /*!< Data read access time 1               */
	volatile uint8_t  NSAC;                 /*!< Data read access time 2 in CLK cycles */
	volatile uint8_t  MaxBusClkFrec;        /*!< Max. bus clock frequency              */
	volatile uint16_t CardComdClasses;      /*!< Card command classes                  */
	volatile uint8_t  RdBlockLen;           /*!< Max. read data block length           */
	volatile uint8_t  PartBlockRead;        /*!< Partial blocks for read allowed       */
	volatile uint8_t  WrBlockMisalign;      /*!< Write block misalignment              */
	volatile uint8_t  RdBlockMisalign;      /*!< Read block misalignment               */
	volatile uint8_t  DSRImpl;              /*!< DSR implemented                       */
	volatile uint8_t  Reserved2;            /*!< Reserved                              */
	volatile uint32_t DeviceSize;           /*!< Device Size                           */
	volatile uint8_t  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min           */
	volatile uint8_t  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max           */
	volatile uint8_t  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min          */
	volatile uint8_t  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max          */
	volatile uint8_t  DeviceSizeMul;        /*!< Device size multiplier                */
	volatile uint8_t  EraseGrSize;          /*!< Erase group size                      */
	volatile uint8_t  EraseGrMul;           /*!< Erase group size multiplier           */
	volatile uint8_t  WrProtectGrSize;      /*!< Write protect group size              */
	volatile uint8_t  WrProtectGrEnable;    /*!< Write protect group enable            */
	volatile uint8_t  ManDeflECC;           /*!< Manufacturer default ECC              */
	volatile uint8_t  WrSpeedFact;          /*!< Write speed factor                    */
	volatile uint8_t  MaxWrBlockLen;        /*!< Max. write data block length          */
	volatile uint8_t  WriteBlockPaPartial;  /*!< Partial blocks for write allowed      */
	volatile uint8_t  Reserved3;            /*!< Reserved                              */
	volatile uint8_t  ContentProtectAppli;  /*!< Content protection application        */
	volatile uint8_t  FileFormatGrouop;     /*!< File format group                     */
	volatile uint8_t  CopyFlag;             /*!< Copy flag (OTP)                       */
	volatile uint8_t  PermWrProtect;        /*!< Permanent write protection            */
	volatile uint8_t  TempWrProtect;        /*!< Temporary write protection            */
	volatile uint8_t  FileFormat;           /*!< File format                           */
	volatile uint8_t  ECC;                  /*!< ECC code                              */
	volatile uint8_t  CSD_CRC;              /*!< CSD CRC                               */
	volatile uint8_t  Reserved4;            /*!< Always 1                              */
}SD_CardCSDTypeDef;

/*
 * Card Identification Data: CID Register
 */
typedef struct
{
	volatile uint8_t  ManufacturerID;  /*!< Manufacturer ID       */
	volatile uint16_t OEM_AppliID;     /*!< OEM/Application ID    */
	volatile uint32_t ProdName1;       /*!< Product Name part1    */
	volatile uint8_t  ProdName2;       /*!< Product Name part2    */
	volatile uint8_t  ProdRev;         /*!< Product Revision      */
	volatile uint32_t ProdSN;          /*!< Product Serial Number */
	volatile uint8_t  Reserved1;       /*!< Reserved1             */
	volatile uint16_t ManufactDate;    /*!< Manufacturing Date    */
	volatile uint8_t  CID_CRC;         /*!< CID CRC               */
	volatile uint8_t  Reserved2;       /*!< Always 1              */
}SD_CardCIDTypeDef;

/* 
 * SD Card Status returned by ACMD13 
 */
typedef struct
{
	volatile uint8_t  DataBusWidth;           /*!< Shows the currently defined data bus width                 */
	volatile uint8_t  SecuredMode;            /*!< Card is in secured mode of operation                       */
	volatile uint16_t CardType;               /*!< Carries information about card type                        */
	volatile uint32_t ProtectedAreaSize;      /*!< Carries information about the capacity of protected area   */
	volatile uint8_t  SpeedClass;             /*!< Carries information about the speed class of the card      */
	volatile uint8_t  PerformanceMove;        /*!< Carries information about the card's performance move      */
	volatile uint8_t  AllocationUnitSize;     /*!< Carries information about the card's allocation unit size  */
	volatile uint16_t EraseSize;              /*!< Determines the number of AUs to be erased in one operation */
	volatile uint8_t  EraseTimeout;           /*!< Determines the timeout for any number of AU erase          */
	volatile uint8_t  EraseOffset;            /*!< Carries information about the erase offset                 */
}SD_CardStatusTypeDef;

/* Exported constants --------------------------------------------------------*/
#define BLOCKSIZE   512U /*!< Block size is 512 bytes */

/*
 * SD Error status enumeration Structure definition 
 */  
#define SD_ERROR_NONE                     SDMMC_ERROR_NONE                    /*!< No error                                                      */
#define SD_ERROR_CMD_CRC_FAIL             SDMMC_ERROR_CMD_CRC_FAIL            /*!< Command response received (but CRC check failed)              */
#define SD_ERROR_DATA_CRC_FAIL            SDMMC_ERROR_DATA_CRC_FAIL           /*!< Data block sent/received (CRC check failed)                   */
#define SD_ERROR_CMD_RSP_TIMEOUT          SDMMC_ERROR_CMD_RSP_TIMEOUT         /*!< Command response timeout                                      */
#define SD_ERROR_DATA_TIMEOUT             SDMMC_ERROR_DATA_TIMEOUT            /*!< Data timeout                                                  */
#define SD_ERROR_TX_UNDERRUN              SDMMC_ERROR_TX_UNDERRUN             /*!< Transmit FIFO underrun                                        */
#define SD_ERROR_RX_OVERRUN               SDMMC_ERROR_RX_OVERRUN              /*!< Receive FIFO overrun                                          */
#define SD_ERROR_ADDR_MISALIGNED          SDMMC_ERROR_ADDR_MISALIGNED         /*!< Misaligned address                                            */
#define SD_ERROR_BLOCK_LEN_ERR            SDMMC_ERROR_BLOCK_LEN_ERR           /*!< Transferred block length is not allowed for the card or the 
																						number of transferred bytes does not match the block length   */
#define SD_ERROR_ERASE_SEQ_ERR            SDMMC_ERROR_ERASE_SEQ_ERR           /*!< An error in the sequence of erase command occurs              */
#define SD_ERROR_BAD_ERASE_PARAM          SDMMC_ERROR_BAD_ERASE_PARAM         /*!< An invalid selection for erase groups                         */
#define SD_ERROR_WRITE_PROT_VIOLATION     SDMMC_ERROR_WRITE_PROT_VIOLATION    /*!< Attempt to program a write protect block                      */
#define SD_ERROR_LOCK_UNLOCK_FAILED       SDMMC_ERROR_LOCK_UNLOCK_FAILED      /*!< Sequence or password error has been detected in unlock 
																						command or if there was an attempt to access a locked card    */
#define SD_ERROR_COM_CRC_FAILED           SDMMC_ERROR_COM_CRC_FAILED          /*!< CRC check of the previous command failed                      */
#define SD_ERROR_ILLEGAL_CMD              SDMMC_ERROR_ILLEGAL_CMD             /*!< Command is not legal for the card state                       */
#define SD_ERROR_CARD_ECC_FAILED          SDMMC_ERROR_CARD_ECC_FAILED         /*!< Card internal ECC was applied but failed to correct the data  */
#define SD_ERROR_CC_ERR                   SDMMC_ERROR_CC_ERR                  /*!< Internal card controller error                                */
#define SD_ERROR_GENERAL_UNKNOWN_ERR      SDMMC_ERROR_GENERAL_UNKNOWN_ERR     /*!< General or unknown error                                      */
#define SD_ERROR_STREAM_READ_UNDERRUN     SDMMC_ERROR_STREAM_READ_UNDERRUN    /*!< The card could not sustain data reading in stream rmode       */
#define SD_ERROR_STREAM_WRITE_OVERRUN     SDMMC_ERROR_STREAM_WRITE_OVERRUN    /*!< The card could not sustain data programming in stream mode    */
#define SD_ERROR_CID_CSD_OVERWRITE        SDMMC_ERROR_CID_CSD_OVERWRITE       /*!< CID/CSD overwrite error                                       */
#define SD_ERROR_WP_ERASE_SKIP            SDMMC_ERROR_WP_ERASE_SKIP           /*!< Only partial address space was erased                         */
#define SD_ERROR_CARD_ECC_DISABLED        SDMMC_ERROR_CARD_ECC_DISABLED       /*!< Command has been executed without using internal ECC          */
#define SD_ERROR_ERASE_RESET              SDMMC_ERROR_ERASE_RESET             /*!< Erase sequence was cleared before executing because an out 
																						of erase sequence command was received                        */
#define SD_ERROR_AKE_SEQ_ERR              SDMMC_ERROR_AKE_SEQ_ERR             /*!< Error in sequence of authentication                           */
#define SD_ERROR_INVALID_VOLTRANGE        SDMMC_ERROR_INVALID_VOLTRANGE       /*!< Error in case of invalid voltage range                        */        
#define SD_ERROR_ADDR_OUT_OF_RANGE        SDMMC_ERROR_ADDR_OUT_OF_RANGE       /*!< Error when addressed block is out of range                    */        
#define SD_ERROR_REQUEST_NOT_APPLICABLE   SDMMC_ERROR_REQUEST_NOT_APPLICABLE  /*!< Error when command request is not applicable                  */  
#define SD_ERROR_PARAM                    SDMMC_ERROR_INVALID_PARAMETER       /*!< the used parameter is not valid                               */  
#define SD_ERROR_UNSUPPORTED_FEATURE      SDMMC_ERROR_UNSUPPORTED_FEATURE     /*!< Error when feature is not insupported                         */
#define SD_ERROR_BUSY                     SDMMC_ERROR_BUSY                    /*!< Error when transfer process is busy                           */ 
#define SD_ERROR_DMA                      SDMMC_ERROR_DMA                     /*!< Error while DMA transfer                                      */
#define SD_ERROR_TIMEOUT                  SDMMC_ERROR_TIMEOUT                 /*!< Timeout error                                                 */
	
/*
 * SD context enumeration
 */ 
#define   SD_CONTEXT_NONE                 0x00000000U  /*!< None                             */
#define   SD_CONTEXT_READ_SINGLE_BLOCK    0x00000001U  /*!< Read single block operation      */
#define   SD_CONTEXT_READ_MULTIPLE_BLOCK  0x00000002U  /*!< Read multiple blocks operation   */
#define   SD_CONTEXT_WRITE_SINGLE_BLOCK   0x00000010U  /*!< Write single block operation     */
#define   SD_CONTEXT_WRITE_MULTIPLE_BLOCK 0x00000020U  /*!< Write multiple blocks operation  */
#define   SD_CONTEXT_IT                   0x00000008U  /*!< Process in Interrupt mode        */
#define   SD_CONTEXT_DMA                  0x00000080U  /*!< Process in DMA mode              */  

/* 
 * SD Supported Memory Cards
 */
#define CARD_SDSC                  0x00000000U
#define CARD_SDHC_SDXC             0x00000001U
#define CARD_SECURED               0x00000003U
		
/* 
 * SD Supported Version
 */
#define CARD_V1_X                  0x00000000U
#define CARD_V2_X                  0x00000001U
	
/* Exported macro ------------------------------------------------------------*/
/*
 * Enable the SD device.
 */ 
#define SD_ENABLE(__HANDLE__) __SDIO_ENABLE(SDIO)

/*
 * Disable the SD device.
 */
#define SD_DISABLE(__HANDLE__) __SDIO_DISABLE(SDIO)

/*
 * Enable the SDMMC DMA transfer.
 */ 
#define SD_DMA_ENABLE(__HANDLE__) __SDIO_DMA_ENABLE(SDIO)

/*
 * Disable the SDMMC DMA transfer.
 */
#define SD_DMA_DISABLE(__HANDLE__)  __SDIO_DMA_DISABLE(SDIO)
 
/*
 * Enable the SD device interrupt.
 * 		SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
 * 		SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
 * 		SDIO_IT_CTIMEOUT: Command response timeout interrupt
 * 		SDIO_IT_DTIMEOUT: Data timeout interrupt
 * 		SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
 * 		SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
 * 		SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
 * 		SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
 * 		SDIO_IT_DATAEND:  Data end (data counter, SDIDCOUNT, is zero) interrupt
 * 		SDIO_IT_DBCKEND:  Data block sent/received (CRC check passed) interrupt
 * 		SDIO_IT_CMDACT:   Command transfer in progress interrupt
 * 		SDIO_IT_TXACT:    Data transmit in progress interrupt
 * 		SDIO_IT_RXACT:    Data receive in progress interrupt
 * 		SDIO_IT_TXFIFOHE: Transmit FIFO Half Empty interrupt
 * 		SDIO_IT_RXFIFOHF: Receive FIFO Half Full interrupt
 * 		SDIO_IT_TXFIFOF:  Transmit FIFO full interrupt
 * 		SDIO_IT_RXFIFOF:  Receive FIFO full interrupt
 * 		SDIO_IT_TXFIFOE:  Transmit FIFO empty interrupt
 * 		SDIO_IT_RXFIFOE:  Receive FIFO empty interrupt
 * 		SDIO_IT_TXDAVL:   Data available in transmit FIFO interrupt
 * 		SDIO_IT_RXDAVL:   Data available in receive FIFO interrupt
 * 		SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
 */
#define SD_ENABLE_IT(__INTERRUPT__) __SDIO_ENABLE_IT(SDIO, (__INTERRUPT__))

/*
 * Disable the SD device interrupt
 * 		SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
 * 		SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
 * 		SDIO_IT_CTIMEOUT: Command response timeout interrupt
 * 		SDIO_IT_DTIMEOUT: Data timeout interrupt
 * 		SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
 * 		SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
 * 		SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
 * 		SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
 * 		SDIO_IT_DATAEND:  Data end (data counter, SDIDCOUNT, is zero) interrupt
 * 		SDIO_IT_DBCKEND:  Data block sent/received (CRC check passed) interrupt
 * 		SDIO_IT_CMDACT:   Command transfer in progress interrupt
 * 		SDIO_IT_TXACT:    Data transmit in progress interrupt
 * 		SDIO_IT_RXACT:    Data receive in progress interrupt
 * 		SDIO_IT_TXFIFOHE: Transmit FIFO Half Empty interrupt
 * 		SDIO_IT_RXFIFOHF: Receive FIFO Half Full interrupt
 * 		SDIO_IT_TXFIFOF:  Transmit FIFO full interrupt
 * 		SDIO_IT_RXFIFOF:  Receive FIFO full interrupt
 * 		SDIO_IT_TXFIFOE:  Transmit FIFO empty interrupt
 * 		SDIO_IT_RXFIFOE:  Receive FIFO empty interrupt
 * 		SDIO_IT_TXDAVL:   Data available in transmit FIFO interrupt
 * 		SDIO_IT_RXDAVL:   Data available in receive FIFO interrupt
 * 		SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt   
 */
#define SD_DISABLE_IT(__INTERRUPT__) __SDIO_DISABLE_IT(SDIO, (__INTERRUPT__))

/*
 * Check whether the specified SD flag is set or not
 *		SDIO_FLAG_CCRCFAIL: Command response received (CRC check failed)
 *		SDIO_FLAG_DCRCFAIL: Data block sent/received (CRC check failed)
 *		SDIO_FLAG_CTIMEOUT: Command response timeout
 *		SDIO_FLAG_DTIMEOUT: Data timeout
 *		SDIO_FLAG_TXUNDERR: Transmit FIFO underrun error
 *		SDIO_FLAG_RXOVERR:  Received FIFO overrun error
 *		SDIO_FLAG_CMDREND:  Command response received (CRC check passed)
 *		SDIO_FLAG_CMDSENT:  Command sent (no response required)
 *		SDIO_FLAG_DATAEND:  Data end (data counter, SDIDCOUNT, is zero)
 *		SDIO_FLAG_DBCKEND:  Data block sent/received (CRC check passed)
 *		SDIO_FLAG_CMDACT:   Command transfer in progress
 *		SDIO_FLAG_TXACT:    Data transmit in progress
 *		SDIO_FLAG_RXACT:    Data receive in progress
 *		SDIO_FLAG_TXFIFOHE: Transmit FIFO Half Empty
 *		SDIO_FLAG_RXFIFOHF: Receive FIFO Half Full
 *		SDIO_FLAG_TXFIFOF:  Transmit FIFO full
 *		SDIO_FLAG_RXFIFOF:  Receive FIFO full
 *		SDIO_FLAG_TXFIFOE:  Transmit FIFO empty
 *		SDIO_FLAG_RXFIFOE:  Receive FIFO empty
 *		SDIO_FLAG_TXDAVL:   Data available in transmit FIFO
 *		SDIO_FLAG_RXDAVL:   Data available in receive FIFO
 *		SDIO_FLAG_SDIOIT:   SD I/O interrupt received
 */
#define SD_GET_FLAG(__FLAG__) __SDIO_GET_FLAG(SDIO, (__FLAG__))

/*
 * Clear the SD's pending flags.
 *		SDIO_FLAG_CCRCFAIL: Command response received (CRC check failed)
 *		SDIO_FLAG_DCRCFAIL: Data block sent/received (CRC check failed)
 *		SDIO_FLAG_CTIMEOUT: Command response timeout
 *		SDIO_FLAG_DTIMEOUT: Data timeout
 *		SDIO_FLAG_TXUNDERR: Transmit FIFO underrun error
 *		SDIO_FLAG_RXOVERR:  Received FIFO overrun error
 *		SDIO_FLAG_CMDREND:  Command response received (CRC check passed)
 *		SDIO_FLAG_CMDSENT:  Command sent (no response required)
 *		SDIO_FLAG_DATAEND:  Data end (data counter, SDIDCOUNT, is zero)
 *		SDIO_FLAG_DBCKEND:  Data block sent/received (CRC check passed)
 *		SDIO_FLAG_SDIOIT:   SD I/O interrupt received
 */
#define SD_CLEAR_FLAG(__FLAG__) __SDIO_CLEAR_FLAG(SDIO, (__FLAG__))

/*
 * Check whether the specified SD interrupt has occurred or not
 *		SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
 *		SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
 *		SDIO_IT_CTIMEOUT: Command response timeout interrupt
 *		SDIO_IT_DTIMEOUT: Data timeout interrupt
 *		SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
 *		SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
 *		SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
 *		SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
 *		SDIO_IT_DATAEND:  Data end (data counter, SDIDCOUNT, is zero) interrupt
 *		SDIO_IT_DBCKEND:  Data block sent/received (CRC check passed) interrupt
 *		SDIO_IT_CMDACT:   Command transfer in progress interrupt
 *		SDIO_IT_TXACT:    Data transmit in progress interrupt
 *		SDIO_IT_RXACT:    Data receive in progress interrupt
 *		SDIO_IT_TXFIFOHE: Transmit FIFO Half Empty interrupt
 *		SDIO_IT_RXFIFOHF: Receive FIFO Half Full interrupt
 *		SDIO_IT_TXFIFOF:  Transmit FIFO full interrupt
 *		SDIO_IT_RXFIFOF:  Receive FIFO full interrupt
 *		SDIO_IT_TXFIFOE:  Transmit FIFO empty interrupt
 *		SDIO_IT_RXFIFOE:  Receive FIFO empty interrupt
 *		SDIO_IT_TXDAVL:   Data available in transmit FIFO interrupt
 *		SDIO_IT_RXDAVL:   Data available in receive FIFO interrupt
 *		SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
 */
#define SD_GET_IT(__INTERRUPT__) __SDIO_GET_IT(SDIO, (__INTERRUPT__))

/*
 * Clear the SD's interrupt pending bits
 *		SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
 *		SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
 *		SDIO_IT_CTIMEOUT: Command response timeout interrupt
 *		SDIO_IT_DTIMEOUT: Data timeout interrupt
 *		SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
 *		SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
 *		SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
 *		SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
 *		SDIO_IT_DATAEND:  Data end (data counter, SDMMC_DCOUNT, is zero) interrupt
 *		SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
 */
#define SD_CLEAR_IT(__INTERRUPT__) __SDIO_CLEAR_IT(SDIO, (__INTERRUPT__))

/* Exported functions --------------------------------------------------------*/
	
/*
 * Initialization and de-initialization functions
 */
uint32_t SD_Init();
uint32_t SD_InitCard();
	
/*
 * Input and Output operation functions
 */
//~ int32_t SD_ReadBlocks_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks);
//~ int32_t SD_WriteBlocks_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks);

//~ void SD_IRQHandler();

/* Callback in non blocking modes (DMA) */
//~ void SD_TxCpltCallback();
//~ void SD_RxCpltCallback();
//~ void SD_ErrorCallback();
//~ void SD_AbortCallback();
	
/*
 * Peripheral Control functions
 */
//~ int32_t SD_ConfigWideBusOperation(uint32_t WideMode);

/*
 * SD card related functions
 */
uint32_t       SD_SendSDStatus(uint32_t *pSDstatus);
//~ SD_CardStateTypeDef SD_GetCardState();
uint32_t       SD_GetCardCID(SD_CardCIDTypeDef *pCID);
uint32_t       SD_GetCardCSD(SD_CardCSDTypeDef *pCSD);
//~ int32_t       SD_GetCardStatus(SD_Cardint32_t *pStatus);
//~ int32_t       SD_GetCardInfo(SD_CardInfoTypeDef *pCardInfo);

/* 
 * Peripheral State and Errors functions
 */
//~ SD_StateTypeDef SD_GetState();
//~ uint32_t SD_GetError();

/*
 * Perioheral Abort management
 */
//~ int32_t SD_Abort();
//~ int32_t SD_Abort_IT();

#endif /* _SD_CARD_H_ */ 
