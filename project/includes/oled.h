#ifndef _OLED_H_
#define _OLED_H_

#include "stdint.h"

#define OLED_WIDTH		128
#define OLED_HEIGTH		64

void oled_init(void);
void oled_set_page_start_address(uint8_t add);
void oled_set_column_start_address(uint8_t add);
void oled_set_contrast(uint8_t value);
void oled_draw_image_at_xy(uint8_t* img, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void oled_clear_display();

#endif // _OLED_H_
