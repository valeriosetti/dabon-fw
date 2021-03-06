#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "stdint.h"

#define EEPROM_PAGE_SIZE_IN_BYTES      256
#define MAX_PARTITION_NAME_LENGTH		16

#pragma pack(1)
typedef struct {
	char name[MAX_PARTITION_NAME_LENGTH];
	uint32_t start_page;
    uint32_t final_page;
    uint32_t data_size;
	uint32_t checksum;
} PARTITION_INFO;
#pragma pop

// public functions
void eeprom_init(void);
int32_t eeprom_read_jedec_id(void);
int32_t eeprom_page_read(uint32_t start_address, uint8_t* data);
PARTITION_INFO* eeprom_get_partition_infos(char* partition_name);
int32_t eeprom_read_page_from_partition(uint16_t page_num);

// shell commands
int eeprom_program_firmware(int argc, char *argv[]);
int eeprom_show_partition_table(int argc, char *argv[]);

#endif	//_EEPROM_H_
