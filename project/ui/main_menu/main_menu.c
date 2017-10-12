#include "kernel.h"
#include "sd_card_icon.xbm"
#include "fm_radio_icon.xbm"
#include "dab_radio_icon.xbm"
#include "left_arrow.xbm"
#include "right_arrow.xbm"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"

#define debug_msg(format, ...)		debug_printf("[main_menu] " format, ##__VA_ARGS__)

ALLOCATE_TASK(main_menu, 200);

uint8_t current_item = 0;
unsigned char* icons[] = {
		fm_radio_icon_bits,
		dab_radio_icon_bits,
		sd_card_icon_bits,
	};

uint8_t received_key;

void main_menu_key_event(uint8_t key, uint8_t event)
{
	debug_msg("key %d - event %d\n", key, event);

	// Ignore release events
	if (event == KEY_RELEASED)
		return;

	received_key = key;
	kernel_activate_task_immediately(&main_menu_task);
}

void main_menu_init()
{
	kernel_init_task(&main_menu_task);
	kernel_activate_task_after_ms(&main_menu_task, 2000);

	buttons_register_key_event_callback(&main_menu_key_event);
}

int32_t main_menu_task_func(void* arg)
{
	// Process key
	if (received_key == KEY_LEFT) {
		current_item = (current_item == 0) ? array_size(icons) : current_item-1;
	} else if (received_key == KEY_RIGHT) {
		current_item = (current_item == array_size(icons)) ? 0 : current_item+1;
	}

	// Draw left and right arrows
	oled_draw_image_at_xy(left_arrow_bits, 0, 0, right_arrow_width, right_arrow_height);
	oled_draw_image_at_xy(right_arrow_bits, OLED_WIDTH - right_arrow_width, 0, right_arrow_width, right_arrow_height);

	// Draw current item on the center
	oled_draw_image_at_xy(icons[current_item], OLED_WIDTH - dab_radio_icon_width/2, 0, dab_radio_icon_width, dab_radio_icon_height);

	return WAIT_FOR_RESUME;
}
