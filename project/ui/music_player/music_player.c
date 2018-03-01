#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "music_player.h"
#include "file_browser.h"
#include "ff.h"
#include "string.h"
#include "mp3_player.h"
#include "file_manager.h"

#define debug_msg(format, ...)		debug_printf("[music_player] " format, ##__VA_ARGS__)

ALLOCATE_TASK(music_player, 200);

uint8_t received_key;
uint16_t current_file_index;

char local_path[MAX_PATH_LENGTH] = "";

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
 * Activate the module
 */
void music_player_start(uint16_t file_index)
{
	buttons_register_key_event_callback(&music_player_key_event);
	kernel_activate_task_immediately(&music_player_task);

	current_file_index = file_index;

	oled_clear_display();
	oled_print_text_at_xy("Test tone", 0, 0);

	received_key = KEY_NONE;
}

/*
 * Concatenate the current folder's path to the file that should be played
 */
static int32_t music_player_prepare_path_for_playback()
{
	memset(local_path, '\0', sizeof(local_path));
	strncpy(local_path, file_manager_get_curr_path_pointer(), MAX_PATH_LENGTH);
	uint16_t local_path_len = strlen(local_path);
	if (local_path[local_path_len] != '/') {
		local_path[local_path_len] = '/';
		local_path_len ++;
	}
	uint16_t remaining_space = MAX_PATH_LENGTH - local_path_len;
	strncpy(&local_path[local_path_len], file_manager_get_item_name(current_file_index), remaining_space);

	return 0;
}

/*
 * Main task
 */
int32_t music_player_task_func()
{
	// Process key
	if (received_key != KEY_NONE) {
		if (received_key == KEY_OK) {
			// If the music is stopped then start it
			if (mp3_player_get_status() == MP3_PLAYER_PAUSED) {
				debug_msg("resuming playback\n");
				mp3_player_resume();
			} else if (mp3_player_get_status() == MP3_PLAYER_IDLE) {
				if (music_player_prepare_path_for_playback() >= 0) {
					debug_msg("starting playback\n");
					mp3_player_play(local_path);
				} else {
					debug_msg("error creating playback path\n");
				}
			}
		} else if (received_key == KEY_CANCEL) {
			// if there's something playing then pause it, otherwise
			// return to the file_browser menu
			if (mp3_player_get_status() == MP3_PLAYER_PLAYING) {
				mp3_player_pause();
				debug_msg("playback paused\n");
			} else {
				buttons_remove_key_event_callback();
				file_browser_start();
				return DIE;
			}
		}
		received_key = KEY_NONE;
	}
}

