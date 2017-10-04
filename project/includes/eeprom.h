#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "stdint.h"

void eeprom_init(void);

int32_t eeprom_read_jedec_id(void);
int32_t eeprom_page_read(uint32_t start_address, uint8_t* data);
int32_t eeprom_page_program(uint32_t start_address, uint8_t* data);
int32_t eeprom_page_erase(uint32_t start_address);

int eeprom_program_firmware(int argc, char *argv[])
;int eeprom_show_partition_table(int argc, char *argv[]);

#endif	//_EEPROM_H_
