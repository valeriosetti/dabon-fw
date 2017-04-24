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

// Macros
#define debug_msg(...)		debug_printf_with_tag("[Eeprom] ", __VA_ARGS__)

#define eeprom_set_hold_pin()			SET_BIT(GPIOE->BSRR, GPIO_BSRR_BR15)
#define eeprom_release_hold_pin()		SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS15)

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

// Private functions
static int eeprom_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size);
static int eeprom_wait_until_busy(void);

static int eeprom_read_jedec_id(void);
static int eeprom_write_enable(void);
static int eeprom_write_disable(void);
static int eeprom_read_data(uint32_t start_address, uint32_t len);
static int eeprom_read_status_registers(uint8_t* status_reg_1, uint8_t* status_reg_2);
static int eeprom_page_program(uint32_t start_address, uint32_t length);
static int eeprom_sector_erase(uint32_t start_address, uint32_t count);
static int eeproc_write_status_reg(uint8_t status_reg_1, uint8_t status_reg_2);

// status register 1 bits
#define STATUS_REG_BUSY			0x01
#define STATUS_REG_WEL			0x02

// Global variables
#define IN_OUT_BUFF_SIZE		256
uint8_t data_out[IN_OUT_BUFF_SIZE];
uint8_t data_in[IN_OUT_BUFF_SIZE];

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

	// Test EEPROM
	eeprom_read_jedec_id();
	eeprom_read_status_registers(&status_reg_1, &status_reg_2);
	eeproc_write_status_reg(0, 0);
	eeprom_read_status_registers(&status_reg_1, &status_reg_2);
	eeprom_write_enable();
	eeprom_read_status_registers(&status_reg_1, NULL);
	eeprom_sector_erase(0UL, 1);
	eeprom_read_status_registers(&status_reg_1, NULL);
	eeprom_write_enable();
	eeprom_read_status_registers(&status_reg_1, NULL);
	eeprom_page_program(0UL, 128);
	eeprom_read_status_registers(&status_reg_1, NULL);
	eeprom_read_data(0UL, 128);
}

/*
 *
 */
static int eeprom_send_cmd(uint8_t* data_out, uint32_t data_out_size, uint8_t* data_in, uint32_t data_in_size)
{
	spi_set_eeprom_CS();
	timer_wait_us(1);
	if (data_out != NULL)
		spi_write(data_out, data_out_size);
	if (data_in != NULL)
		spi_read(data_in, data_in_size);
	timer_wait_us(1);
	spi_release_eeprom_CS();

	if (data_in != NULL) {
		int i;
		for (i=0; i<data_in_size; i++) {
			debug_msg("  rsp byte %d = 0x%x\n", i, data_in[i]);
		}
	}
}

/*
 *
 */
static int eeprom_wait_until_busy(void)
{
	// wait for the BUSY flag to be cleared
	uint8_t status_reg_1;
	do {
		eeprom_read_status_registers(&status_reg_1, NULL);
	} while(status_reg_1 & STATUS_REG_BUSY);
}

/*
 *
 */
static int eeprom_read_jedec_id()
{
	data_out[0] = EEPROM_CMD_READ_JEDEC_ID;

	debug_msg("read_jedec_id\n");
	eeprom_send_cmd(data_out, 1, data_in, 4);
}

/*
 *
 */
static int eeprom_write_enable()
{
	data_out[0] = EEPROM_CMD_WRITE_ENABLE;

	debug_msg("write_enable\n");
	eeprom_send_cmd(data_out, 1, NULL, 0);
}

/*
 *
 */
static int eeprom_write_disable()
{
	data_out[0] = EEPROM_CMD_WRITE_DISABLE;

	debug_msg("write_disable\n");
	eeprom_send_cmd(data_out, 1, NULL, 0);
}

/*
 *
 */
static int eeprom_read_data(uint32_t start_address, uint32_t len)
{
	data_out[0] = EEPROM_CMD_READ_DATA;
	data_out[1] = ((start_address & 0x00FF0000) >> 16);
	data_out[2] = ((start_address & 0x0000FF00) >> 8);
	data_out[3] = ((start_address & 0x000000FF) >> 0);

	debug_msg("read_data\n");
	// TODO: store the read data somewhere...
	eeprom_send_cmd(data_out, 4, data_in, len);
}

/*
 *
 */
static int eeprom_read_status_registers(uint8_t* status_reg_1, uint8_t* status_reg_2)
{
	if (status_reg_1 != NULL) {
		debug_msg("read_status_register 1\n");
		data_out[0] = EEPROM_CMD_READ_STATUS_REG_1;
		eeprom_send_cmd(data_out, 1, data_in, 1);
		*status_reg_1 = data_in[0];
	}

	if (status_reg_2 != NULL) {
		debug_msg("read_status_register 2\n");
		data_out[0] = EEPROM_CMD_READ_STATUS_REG_2;
		eeprom_send_cmd(data_out, 1, data_in, 1);
		*status_reg_2 = data_in[0];
	}
}

/*
 *
 */
static int eeproc_write_status_reg(uint8_t status_reg_1, uint8_t status_reg_2)
{
	data_out[0] = EEPROM_CMD_WRITE_STATUS_REG;
	data_out[1] = status_reg_1;
	data_out[2] = status_reg_2;

	debug_msg("write_status_reg\n");
	eeprom_send_cmd(data_out, 3, NULL, 0);
}

/*
 *
 */
static int eeprom_page_program(uint32_t start_address, uint32_t len)
{
	data_out[0] = EEPROM_CMD_PAGE_PROGRAM;
	data_out[1] = ((start_address & 0x00FF0000) >> 16);
	data_out[2] = ((start_address & 0x0000FF00) >> 8);
	data_out[3] = ((start_address & 0x000000FF) >> 0);

	// TODO: import data from outside...
	int i;
	for (i=0; i<len; i++) {
		data_out[i+4] = (uint8_t) i;
	}

	debug_msg("page_program\n");
	eeprom_send_cmd(data_out, len+4, NULL, 0);

	eeprom_wait_until_busy();
}

/*
 *
 */
static int eeprom_sector_erase(uint32_t start_address, uint32_t length)
{
	data_out[0] = EEPROM_CMD_SECTOR_ERASE;
	data_out[1] = ((start_address & 0x00FF0000) >> 16);
	data_out[2] = ((start_address & 0x0000FF00) >> 8);
	data_out[3] = ((start_address & 0x000000FF) >> 0);

	debug_msg("sector_erase\n");
	eeprom_send_cmd(data_out, 4, NULL, 0);

	eeprom_wait_until_busy();
}
