/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sd_card.h"

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	(void)pdrv;
	DSTATUS stat;
	int result;

	if (SD_GetCardState() != SD_CARD_ERROR)
		return RES_OK;
		
	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	(void)pdrv;

	if (SD_InitCard() != 0)
		return STA_NOINIT;
	
	return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
#define DISK_READ_TIMEOUT_MS		5000
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	(void)pdrv;
	
	if (SD_ReadBlocks_DMA(buff, sector, count) != 0)
		return RES_ERROR;

	// wait for the read operation to be completed
	uint32_t start_tick = systick_get_tick_count();
	while (SD_GetContext() != SD_CONTEXT_NONE) {
		if ((systick_get_tick_count()-start_tick) > DISK_READ_TIMEOUT_MS)
			return RES_ERROR;
	}
	
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	(void)pdrv;
	DRESULT res;
	int result;

	// translate the arguments here

	//result = MMC_disk_write(buff, sector, count);

	// translate the reslut code here

	return res;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	(void)pdrv;
	SD_CardInfoTypeDef card_info;

	switch (cmd) {
		case CTRL_SYNC :
			return RES_OK;
			
		case GET_SECTOR_COUNT:
			SD_GetCardInfo(&card_info);
			*(DWORD*)buff = card_info.BlockNbr;
			return RES_OK;

		case GET_SECTOR_SIZE :
			*(WORD*)buff = BLOCKSIZE;
			return RES_OK;

		case GET_BLOCK_SIZE :
			*(DWORD*)buff = BLOCKSIZE;
			return RES_OK;

		default:
			return RES_PARERR;
	}
}

