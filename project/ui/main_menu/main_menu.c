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

void FmRadioClick(void);
void DabRadioClick(void);
void SdCardClick(void);

typedef struct {
	unsigned char* icon;
	void (*onClick)(void);
}MENU_ITEM;

MENU_ITEM menu_items[] = {
		{fm_radio_icon_bits, FmRadioClick},
		{dab_radio_icon_bits, DabRadioClick},
		{sd_card_icon_bits, SdCardClick},
};

uint8_t received_key;

/*
 * Callback function for key events
 */
void main_menu_key_event(uint8_t key, uint8_t event)
{
	//debug_msg("key %d - event %d\n", key, event);

	// Ignore release events
	if (event == KEY_RELEASED)
		return;

	received_key = key;
	kernel_activate_task_immediately(&main_menu_task);
}

/*
 * Initialization function for the main menu
 */
void main_menu_init()
{
	kernel_init_task(&main_menu_task);
	kernel_activate_task_after_ms(&main_menu_task, 2000);

	buttons_register_key_event_callback(&main_menu_key_event);
}

/*
 * Main menu task
 */
int32_t main_menu_task_func(void* arg)
{
	// Process key
	if ((received_key == KEY_LEFT) || (received_key == KEY_RIGHT)) {
		if (received_key == KEY_LEFT) {
			current_item = (current_item == 0) ? (array_size(menu_items)-1) : current_item-1;
		} else { // KEY_RIGHT
			current_item = (current_item == (array_size(menu_items)-1)) ? 0 : current_item+1;
		}
	} else if (received_key == KEY_OK) {
		//kernel_kill_task(&main_menu_task);
		menu_items[current_item].onClick();
	}

	oled_clear_display();
	// Draw left and right arrows
	oled_draw_image_at_xy(left_arrow_bits, 0, 0, right_arrow_height, right_arrow_width);
	oled_draw_image_at_xy(right_arrow_bits, OLED_WIDTH - right_arrow_height, 0, right_arrow_height, right_arrow_width);
	// Draw current item on the center
	oled_draw_image_at_xy(menu_items[current_item].icon, (OLED_WIDTH-dab_radio_icon_width)/2, 0,
							dab_radio_icon_width, dab_radio_icon_height);

	return WAIT_FOR_RESUME;
}

/*
 *
 */
void FmRadioClick(void)
{
	debug_msg("fm radio click\n");
}

/*
 *
 */
void DabRadioClick(void)
{
	debug_msg("dab radio click\n");
}

/*
 *
 */
void SdCardClick(void)
{
	debug_msg("sd card click\n");
}
