#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "file_browser.h"
#include "main_menu.h"

#define debug_msg(format, ...)		debug_printf("[file_browser] " format, ##__VA_ARGS__)

ALLOCATE_TASK(file_browser, 200);

uint8_t received_key;

/*
 * Initialization function for the module
 */
void file_browser_init()
{
	kernel_init_task(&file_browser_task);
}

/*
 * Callback function for key events
 */
void file_browser_key_event(uint8_t key, uint8_t event)
{
	debug_msg("key %d - event %d\n", key, event);

	// Ignore release events
	if (event == KEY_RELEASED)
		return;

	received_key = key;
	kernel_activate_task_immediately(&file_browser_task);
}

/*
 * Activate the module
 */
void file_browser_start()
{
	buttons_register_key_event_callback(&file_browser_key_event);
	kernel_activate_task_immediately(&file_browser_task);
}

/*
 * Main task
 */
int32_t file_browser_task_func(void* arg)
{
	// Process key
	if ((received_key == KEY_UP) || (received_key == KEY_DOWN)) {

	} else if (received_key == KEY_OK) {

	} else if (received_key == KEY_CANCEL) {
		buttons_remove_key_event_callback();
		main_menu_start();
		return DIE;
	}

	oled_clear_display();

	return WAIT_FOR_RESUME;
}

