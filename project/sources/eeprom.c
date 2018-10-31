/*
 * NOTES
 * The W25Q16DV array is organized into 8,192 programmable pages of 256-bytes each. Up to 256 bytes
 * can be programmed at a time. Pages can be erased in groups of 16 (4KB sector erase), groups of 128
 * (32KB block erase), groups of 256 (64KB block erase) or the entire chip (chip erase).
 */
#include "eeprom.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "timer.h"
#include "debug_printf.h"
#include "utils.h"
#include "clock_configuration.h"
#include "spi.h"
#include "string.h"
#include "stdlib.h"
#include "Si468x.h"

// Macros
#define debug_msg(format, ...)		debug_printf("[eeprom] " format, ##__VA_ARGS__)

#define eeprom_set_hold_pin()			SET_BIT(GPIOE->BSRR, GPIO_BSRR_BR15)
#define eeprom_release_hold_pin()		SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS15)

// Private functions
int32_t eeprom_page_program(uint32_t start_address, uint8_t* data);
int32_t eeprom_page_erase(uint32_t start_address);

/*
 * The EEPROM will be divided into different sections in order to save all the
 * firmwares. The first page (256 bytes) will store informations about the other
 * partitions using the following structures.
 */
#pragma pack(1)
struct {
    PARTITION_INFO bootloader;
    PARTITION_INFO fm_radio;
    PARTITION_INFO dab_radio;
    PARTITION_INFO general_purpose_data;
} eeprom_partitions_table;
#pragma pop

// Commands
#define EEPROM_CMD_WRITE_STATUS_REG		0x01
#define EEPROM_CMD_PAGE_PROGRAM			0x02
#define EEPROM_CMD_READ_DATA			0x03
#define EEPROM_CMD_WRITE_DISABLE		0x04
#define EEPROM_CMD_READ_STATUS_REG_1	0x05
#define EEPROM_CMD_WRITE_ENABLE			0x06
#define EEPROM_CMD_SECTOR_ERASE			0x20
#define EEPROM_CMD_READ_STATUS_REG_2	0x35
#define EEPROM_CMD_BLOCK_ERASE_32KB		0x52
#define EEPROM_CMD_READ_JEDEC_ID		0x9F
#define EEPROM_CMD_BLOCK_ERASE_64KB		0xD8

// status register 1 bits
#define STATUS_REG_BUSY			0x01
#define STATUS_REG_WEL			0x02

// Temporary buffer used to store a page
uint8_t tmp_page[EEPROM_PAGE_SIZE_IN_BYTES];

/***************************************************************/
/*	BASIC FUNCTIONS
/***************************************************************/
/*
 * Basic function for sending/receiving data
 */
static int32_t eeprom_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size)
{
	spi_set_eeprom_CS();
	timer_wait_us(1);
	if (data_out != NULL)
		spi_write(data_out, data_out_size);
	if (data_in != NULL)
		spi_read(data_in, data_in_size);
	timer_wait_us(1);
	spi_release_eeprom_CS();
    
    return 0;
}

/*
 * This is the same as the functions above, but CS is kept asserted when the function terminates
 */
static int32_t eeprom_send_cmd_hold_CS(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size)
{
	spi_set_eeprom_CS();
	timer_wait_us(1);
	if (data_out != NULL)
		spi_write(data_out, data_out_size);
	if (data_in != NULL)
		spi_read(data_in, data_in_size);
	timer_wait_us(1);

    return 0;
}

/*
 * Allows the eeprom to be written
 */
static int32_t eeprom_write_enable()
{
    uint8_t cmd_data[] = {EEPROM_CMD_WRITE_ENABLE};

	eeprom_send_cmd(cmd_data, sizeof(cmd_data), NULL, 0);
    
    return 0;
}

/*
 * Inhibit data changes
 */
static int32_t eeprom_write_disable()
{
    uint8_t cmd_data[] = {EEPROM_CMD_WRITE_DISABLE};
    
	eeprom_send_cmd(cmd_data, sizeof(cmd_data), NULL, 0);
    
    return 0;
}

/*
 * Read status register 1 and/or 2
 */
static int32_t eeprom_read_status_registers(uint8_t* status_reg_1, uint8_t* status_reg_2)
{
    int32_t ret_val = 0; 
	if (status_reg_1 != NULL) {
        uint8_t cmd_data[] = {EEPROM_CMD_READ_STATUS_REG_1};
		ret_val = eeprom_send_cmd(cmd_data, sizeof(cmd_data), status_reg_1, 1);
        if (ret_val != 0)
            return ret_val;
	}

	if (status_reg_2 != NULL) {
        uint8_t cmd_data[] = {EEPROM_CMD_READ_STATUS_REG_2};
		ret_val = eeprom_send_cmd(cmd_data, 1, status_reg_2, 1);
        if (ret_val != 0)
            return ret_val;
	}
    
    return ret_val;
}

/*
 * Wait for the busy flag in polling mode
 */
static int32_t eeprom_wait_until_busy(void)
{
	uint8_t status_reg_1;
    int32_t ret_val = 0;
	do {
		ret_val = eeprom_read_status_registers(&status_reg_1, NULL);
        if (ret_val != 0)
            return ret_val;
	} while(status_reg_1 & STATUS_REG_BUSY);
    
    return ret_val;
}

/*
 * Write the status register
 */
static int32_t eeproc_write_status_reg(uint8_t status_reg_1, uint8_t status_reg_2)
{
    uint8_t cmd_data[] = {EEPROM_CMD_WRITE_STATUS_REG, status_reg_1, status_reg_2};

	return eeprom_send_cmd(cmd_data, sizeof(cmd_data), NULL, 0);
}

/*
 *
 */
int32_t eeprom_read_jedec_id()
{
	uint8_t cmd_data[] = {EEPROM_CMD_READ_JEDEC_ID};
    uint8_t jedec_info[4];

    if (eeprom_send_cmd(cmd_data, sizeof(cmd_data), jedec_info, sizeof(jedec_info)) == 0) {
        debug_msg("jedec code = 0x%x, 0x%x, 0x%x, 0x%x\n", jedec_info[0], jedec_info[1], jedec_info[2], jedec_info[3]);
        return 0;
    } else {
        debug_msg("error reading jedec code\n");
        return -1;
    } 
}

/***************************************************************/
/*	PAGE READ/WRITE/ERASE
/***************************************************************/
/*
 *
 */
int32_t eeprom_page_read(uint32_t start_address, uint8_t* data)
{
    uint8_t cmd_data[] = {
        EEPROM_CMD_READ_DATA,
        ((start_address & 0x00FF0000) >> 16),
        ((start_address & 0x0000FF00) >> 8),
        ((start_address & 0x000000FF) >> 0) 
    };

	if (eeprom_send_cmd(cmd_data, sizeof(cmd_data), data, EEPROM_PAGE_SIZE_IN_BYTES) >= 0) {
        return 0;
    } 
    
    debug_msg("error reading page num = %d\n", start_address);
    return -1;
}

/*
 *
 */
int32_t eeprom_page_program(uint32_t start_address, uint8_t* data)
{
 	uint8_t cmd_data[] = { 
        EEPROM_CMD_PAGE_PROGRAM,
        ((start_address & 0x00FF0000) >> 16),
        ((start_address & 0x0000FF00) >> 8),
        ((start_address & 0x000000FF) >> 0) 
    };

	if (eeprom_send_cmd_hold_CS(cmd_data, sizeof(cmd_data), NULL, 0) == 0) {
        if (eeprom_send_cmd(data, EEPROM_PAGE_SIZE_IN_BYTES, NULL, 0) == 0) {
            eeprom_wait_until_busy();
            return 0;
        }
    }
    
    debug_msg("error programming page num = %d\n", start_address);
    return -1;
}

/*
 *
 */
int32_t eeprom_page_erase(uint32_t start_address)
{
    uint8_t cmd_data[] = { 
        EEPROM_CMD_SECTOR_ERASE,
        ((start_address & 0x00FF0000) >> 16),
        ((start_address & 0x0000FF00) >> 8),
        ((start_address & 0x000000FF) >> 0)
    };

	if (eeprom_send_cmd(cmd_data, sizeof(cmd_data), NULL, 0) == 0) {
        eeprom_wait_until_busy();
        return 0;
    } else {
        debug_msg("error erasing page num = %d\n", start_address);
        return -1;
    }
}

/***************************************************************/
/*	PARTITION TABLE MANAGING
/***************************************************************/
/*
 * Create a local copy of the eeprom's partition table
 */
int32_t eeprom_get_partition_table()
{
    // Read the EEPROM content
    if (eeprom_page_read(0, tmp_page) >= 0) {
        memcpy(&eeprom_partitions_table, tmp_page, sizeof(eeprom_partitions_table));
        return 0;
    } else {
        debug_msg("error reading the partiton table\n");
        return -1;
    }
}

/*
 * Copy the local version of the partition table to the EEPROM's 1st page
 */
int32_t eeprom_update_partition_table()
{
	int32_t ret_val;
	memset(tmp_page, 0, sizeof(tmp_page));
	memcpy(tmp_page, &eeprom_partitions_table, sizeof(eeprom_partitions_table));

	eeprom_write_enable();
    if (eeprom_page_program(0, tmp_page) >= 0) {
    	ret_val = 0;
    } else {
        debug_msg("error writing the partiton table\n");
        ret_val = -1;
    }

    eeprom_write_disable();
    return ret_val;
}

/*
 *
 */
int eeprom_show_partition_table(int argc, char *argv[])
{
	PARTITION_INFO* tmp_partition = (PARTITION_INFO*)&eeprom_partitions_table;

	while (tmp_partition->name[0] != 0xFF) {
		debug_msg("partition_name = %s\n", tmp_partition->name);
		debug_msg("start page = %d\n", tmp_partition->start_page);
		debug_msg("end page = %d\n",tmp_partition->final_page);
		debug_msg("data size = %d\n", tmp_partition->data_size);
		debug_msg("checksum = 0x%x\n", tmp_partition->checksum);
		tmp_partition++;
	}
}

/***************************************************************/
/*	GENERIC MODULE'S FUNCTIONS
/***************************************************************/
/*
 * Initialize the eeprom
 */
void eeprom_init()
{
	uint8_t status_reg_1, status_reg_2;

	// Configure GPIOs
	//	PE15 -> HOLD (output, push-pull, high at startup)
	RCC_GPIOE_CLK_ENABLE();
	eeprom_release_hold_pin();
	MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODE15_Msk, MODER_GENERAL_PURPOSE_OUTPUT << GPIO_MODER_MODE15_Pos);
	MODIFY_REG(GPIOE->OSPEEDR, GPIO_MODER_MODE15_Msk, OSPEEDR_50MHZ << GPIO_MODER_MODE15_Pos);
    
	eeprom_read_jedec_id();
    eeprom_get_partition_table();
}

PARTITION_INFO* eeprom_get_partition_infos(char* partition_name)
{
	PARTITION_INFO* tmp_partition = (PARTITION_INFO*)&eeprom_partitions_table;

	while (tmp_partition->name[0] != 0xFF) {
		if (strcmp(tmp_partition->name, partition_name) == 0) {
			return tmp_partition;
		}
	}
	return NULL;
}

/***************************************************************/
/*	EEPROM PROGRAMMING
/***************************************************************/
/*
 * Copy a firmware image from the STM32's memory to the EEPROM
 */
int32_t eeprom_copy_firmware(uint8_t* fw_image, uint32_t img_size, uint32_t start_page)
{
	uint16_t page_len;
	int32_t ret_val = 0;

	eeprom_write_enable();

	while (img_size > 0) {
		// Copy to the local buffer the data that must be written on the physical page
		memset(tmp_page, 0, sizeof(tmp_page));
		page_len = (img_size > EEPROM_PAGE_SIZE_IN_BYTES) ? EEPROM_PAGE_SIZE_IN_BYTES : img_size;
		memcpy(tmp_page, fw_image, page_len);
		fw_image += page_len;
		img_size -= page_len;
		// Erase and program the eeprom with the new data
		if (eeprom_page_erase(start_page) != 0) {
			ret_val = -1;
			break;
		}
		if (eeprom_page_program(start_page, tmp_page) != 0){
			ret_val = -1;
			break;
		}
		start_page++;
	}

	if (ret_val >= 0)
		ret_val = start_page;

	eeprom_write_disable();
	return ret_val;
}

/*
 *
 */
int eeprom_program_firmware(int argc, char *argv[])
{
    if (argc != 2){
    	debug_msg("Wrong number of parameters\n");
    	return -1;
    }

    uint32_t start_page = atoi(argv[1]);
    uint32_t final_page;

    if (strcmp(argv[0],"boot") == 0) {
    	final_page = eeprom_copy_firmware(&_binary___external_firmwares_rom00_patch_016_bin_start, sizeof_binary_image(rom00_patch_016_bin), start_page);
    	strcpy(eeprom_partitions_table.bootloader.name, "bootloader");
    	eeprom_partitions_table.bootloader.start_page = start_page;
    	eeprom_partitions_table.bootloader.data_size = sizeof_binary_image(rom00_patch_016_bin);
    	eeprom_partitions_table.bootloader.final_page = final_page;
    	eeprom_update_partition_table();
    } else if (strcmp(argv[0],"fm") == 0) {
        if (sizeof_binary_image(fmhd_radio_5_0_4_bin) < 2*sizeof(uint8_t)) {
            debug_msg("Error: the STM32 image does not include the FM firmware\n");
            return -1;
        }
        final_page = eeprom_copy_firmware(&_binary___external_firmwares_fmhd_radio_5_0_4_bin_start, sizeof_binary_image(fmhd_radio_5_0_4_bin), start_page);
    	strcpy(eeprom_partitions_table.fm_radio.name, "fm_radio");
    	eeprom_partitions_table.fm_radio.start_page = start_page;
    	eeprom_partitions_table.fm_radio.data_size = sizeof_binary_image(fmhd_radio_5_0_4_bin);
    	eeprom_partitions_table.fm_radio.final_page = final_page;
    	eeprom_update_partition_table();
    } else if (strcmp(argv[0],"dab") == 0) {
        if (sizeof_binary_image(dab_radio_5_0_5_bin) < 2*sizeof(uint8_t)) {
            debug_msg("Error: the STM32 image does not include the DAB firmware\n");
            return -1;
        }
        final_page = eeprom_copy_firmware(&_binary___external_firmwares_dab_radio_5_0_5_bin_start, sizeof_binary_image(dab_radio_5_0_5_bin), start_page);
		strcpy(eeprom_partitions_table.dab_radio.name, "dab_radio");
		eeprom_partitions_table.dab_radio.start_page = start_page;
		eeprom_partitions_table.dab_radio.data_size = sizeof_binary_image(dab_radio_5_0_5_bin);
		eeprom_partitions_table.dab_radio.final_page = final_page;
    } else {
        debug_msg("unknown firmware\n");
        return -1;
    }
    
    return 0;
}
