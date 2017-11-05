#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "music_player.h"
#include "file_browser.h"
#include "ff.h"
#include "string.h"

#define debug_msg(format, ...)		debug_printf("[music_player] " format, ##__VA_ARGS__)

ALLOCATE_TASK(music_player, 200);

uint8_t received_key;

/*
 * Initialization function for the module
 */
void music_player_init()
{
	kernel_init_task(&music_player_task);
}

/*
 * Callback function for key events
 */
void music_player_key_event(uint8_t key, uint8_t event)
{
	debug_msg("key %d - event %d\n", key, event);

	// Ignore release events
	if (event == KEY_RELEASED)
		return;

	received_key = key;
	kernel_activate_task_immediately(&music_player_task);
}

/*
 * Exit from this module
 */
static void music_player_stop()
{
	buttons_remove_key_event_callback();
	file_browser_start();
}

/*
 * Activate the module
 */
void music_player_start()
{
	buttons_register_key_event_callback(&music_player_key_event);
	kernel_activate_task_immediately(&music_player_task);

	received_key = KEY_NONE;
}

/*
 * Main task
 */
int32_t music_player_task_func(void* arg)
{
	// Process key
	if (received_key == KEY_OK) {
		
	} else if (received_key == KEY_CANCEL) {
		
	}

	received_key = KEY_NONE;

	return WAIT_FOR_RESUME;
}

