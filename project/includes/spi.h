#ifndef _SPI_H_
#define _SPI_H_

#include "stdint.h"
#include "gpio.h"
#include "utils.h"

// Public functions
void spi_init(void);
int32_t spi_read(uint8_t* data, uint32_t len);
int32_t spi_write(uint8_t* data, uint32_t len);
#define spi_set_eeprom_CS()			SET_BIT(GPIOA->BSRR, GPIO_BSRR_BR0)
#define spi_release_eeprom_CS()		SET_BIT(GPIOA->BSRR, GPIO_BSRR_BS0)
#define spi_set_Si468x_CS()			SET_BIT(GPIOD->BSRR, GPIO_BSRR_BR10)
#define spi_release_Si468x_CS()		SET_BIT(GPIOD->BSRR, GPIO_BSRR_BS10)

// Functions' return values
#define SPI_SUCCESS			0L

#endif //_SPI_H_
