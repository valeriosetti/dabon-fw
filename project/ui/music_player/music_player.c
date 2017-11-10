#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "music_player.h"
#include "file_browser.h"
#include "ff.h"
#include "string.h"
#include "output_i2s.h"

#define debug_msg(format, ...)		debug_printf("[music_player] " format, ##__VA_ARGS__)

ALLOCATE_TASK(music_player, 200);

uint8_t received_key;

int16_t sine_look_up_table[] = {
		0x8000,0x90b5,0xa120,0xb0fb,0xbfff,0xcdeb,0xda82,0xe58c,
		0xeed9,0xf641,0xfba2,0xfee7,0xffff,0xfee7,0xfba2,0xf641,
		0xeed9,0xe58c,0xda82,0xcdeb,0xbfff,0xb0fb,0xa120,0x90b5,
		0x8000,0x6f4a,0x5edf,0x4f04,0x4000,0x3214,0x257d,0x1a73,
		0x1126,0x9be,0x45d,0x118,0x0,0x118,0x45d,0x9be,
		0x1126,0x1a73,0x257d,0x3214,0x4000,0x4f04,0x5edf,0x6f4a
};
#define array_length(_x_)	(sizeof(_x_)/sizeof(_x_[0]))
int16_t local_buffer[2*array_length(sine_look_up_table)];
static void music_player_initialize_buffers()
{
	uint16_t index;
	for (index=0; index<array_length(sine_look_up_table); index++) {
		local_buffer[2*index] = local_buffer[2*index+1] = (int16_t)(sine_look_up_table[index] - 0x8000);
	}
}

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
 * Callback function from the output_i2s
 */
void music_player_request_audio_samples()
{
	kernel_activate_task_immediately(&music_player_task);
}

/*
 * Activate the module
 */
void music_player_start()
{
	buttons_register_key_event_callback(&music_player_key_event);
	kernel_activate_task_immediately(&music_player_task);

	music_player_initialize_buffers();
	oled_clear_display();
	oled_print_text_at_xy("Playing test tone", 0, 0);

	received_key = KEY_NONE;
}

/*
 * Main task
 */
int32_t music_player_task_func()
{
	// Process key
	if (received_key != KEY_NONE) {
		if (received_key == KEY_OK) {
			debug_msg("starting playback\n");
			output_i2s_register_callback(music_player_request_audio_samples);
		} else if (received_key == KEY_CANCEL) {
			output_i2s_register_callback(NULL);
			buttons_remove_key_event_callback();
			file_browser_start();
			return DIE;
		}
		received_key = KEY_NONE;
	}

	// If there's enough space on the output buffer then copy some data
	if (output_i2s_get_buffer_free_space() > array_length(local_buffer)) {
		output_i2s_enqueue_samples(local_buffer, array_length(local_buffer));
	} else {
		debug_msg("wait\n");
		return WAIT_FOR_RESUME;
	}
	// If there's still space, then reschedule immediately, otherwise wait
	// for the callback
	if (output_i2s_get_buffer_free_space() > array_length(local_buffer)) {
		return IMMEDIATELY;
	} else {
		return WAIT_FOR_RESUME;
	}
}

