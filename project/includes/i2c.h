#ifndef _I2C_H_
#define _I2C_H_

#include "stdint.h"

int32_t i2c_init(void);
int32_t i2c_write_buffer(uint8_t addr, uint8_t* data, uint8_t length);
int32_t i2c_read_buffer(uint8_t addr, uint8_t *data, uint8_t length);
int32_t i2c_write_byte(uint8_t addr, uint8_t data);
int32_t i2c_read_byte(uint8_t addr, uint8_t* data);
int32_t i2c_scan_address(uint8_t addr);

#endif /* _I2C_H_ */
