#ifndef _I2C_H_
#define _I2C_H_

#include "stdint.h"

int i2c_init(void);
int i2c_write_buffer(uint8_t addr, uint8_t* data, uint8_t length);
int i2c_read_buffer(uint8_t addr, uint8_t *data, uint8_t length);
int i2c_write_byte(uint8_t addr, uint8_t data);
int i2c_read_byte(uint8_t addr, uint8_t* data);
int i2c_scan_address(uint8_t addr);

// Return values
#define I2C_SUCCESS			0L
#define I2C_ERROR			-1L

#endif /* _I2C_H_ */
