#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include "stdint.h"

void sd_card_init(void);
uint8_t sd_card_is_card_inserted(void);

#endif	//_SD_CARD_H_
