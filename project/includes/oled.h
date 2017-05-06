#ifndef _OLED_H_
#define _OLED_H_

#include "stdint.h"

void oled_init(void);
void oled_set_page_start_address(uint8_t add);
void oled_set_column_start_address(uint8_t add);
void oled_set_contrast(uint8_t value);

#endif // _OLED_H_
