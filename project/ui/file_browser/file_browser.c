#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "file_browser.h"
#include "main_menu.h"
#include "music_player.h"
#include "ff.h"
#include "string.h"
#include "file_manager.h"

#define debug_msg(format, ...)		debug_printf("[file_browser] " format, ##__VA_ARGS__)

ALLOCATE_TASK(file_browser, 200);

uint8_t received_key;
int16_t first_shown_item;
int16_t selected_item;

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
 * Update the shown list of files
 */
#define TITLE_LINE			0
#define FIRST_LIST_LINE		1
#define LAST_LIST_LINE		7
#define MAX_SHOWN_ITEMS		(LAST_LIST_LINE-FIRST_LIST_LINE+1)
static void file_browser_update_shown_files()
{
	oled_clear_display();

	// write the current path on the screen (with inverted colors)
	if (file_manager_is_at_root()) {
		oled_print_text_at_xy("Root folder", FONT_CHAR_WIDTH, TITLE_LINE);
	} else {
		oled_print_text_at_xy(file_manager_get_current_folder_name(), FONT_CHAR_WIDTH, TITLE_LINE);
	}
	oled_invert_line_color(0);

	uint16_t offset_index = 0;
	while (((first_shown_item+offset_index)<file_manager_get_item_count()) && (offset_index<LAST_LIST_LINE)) {
		// Draw a marker before the selected item
		if (first_shown_item+offset_index == selected_item) {
			oled_print_text_at_xy(">", 0, offset_index+1);
		}
		// If it's a folder then put a "[]" symbol before the name
		if (file_manager_is_item_a_dir(selected_item)) {
			oled_print_text_at_xy("[] ", FONT_CHAR_WIDTH, offset_index+1);
			oled_print_text_at_xy(file_manager_get_item_name(first_shown_item+offset_index), FONT_CHAR_WIDTH*4, offset_index+1);
		} else {
			oled_print_text_at_xy(file_manager_get_item_name(first_shown_item+offset_index), FONT_CHAR_WIDTH, offset_index+1);
		}
		offset_index++;
	}

	// add up and down arrow on the right part of the screen
	if (selected_item < file_manager_get_item_count()-1) {
		oled_print_char_at_xy(DOWN_ARROW, 24*FONT_CHAR_WIDTH, LAST_LIST_LINE);
	}
	if (selected_item > 0) {
		oled_print_char_at_xy(UP_ARROW, 24*FONT_CHAR_WIDTH, FIRST_LIST_LINE);
	}
}

/*
 * Exit from this module
 */
static void file_browser_stop()
{
	f_mount(0, "", 0);	// unmount the registered file system
	buttons_remove_key_event_callback();
	main_menu_start();
}

/*
 * Activate the module
 */
void file_browser_start()
{
	buttons_register_key_event_callback(&file_browser_key_event);
	kernel_activate_task_immediately(&file_browser_task);

	if (file_manager_mount_disk() < 0) {
		file_browser_stop();
		return;
	}

	file_browser_update_shown_files();
	received_key = KEY_NONE;
}

/*
 * Main task
 */
int32_t file_browser_task_func()
{
	// Process key
	if (received_key == KEY_DOWN) {
		if ((selected_item < file_manager_get_item_count()-1)) {
			selected_item += 1;
		}
		if ((selected_item - first_shown_item) >= MAX_SHOWN_ITEMS) {
			first_shown_item = selected_item - MAX_SHOWN_ITEMS + 1;
		}
		file_browser_update_shown_files();
	} else if (received_key == KEY_UP) {
		if (selected_item > 0) {
			selected_item -= 1;
		}
		if (selected_item < first_shown_item) {
			first_shown_item = selected_item;
		}
		file_browser_update_shown_files();
	} else if (received_key == KEY_OK) {
		if (file_manager_is_item_a_dir(selected_item)) {
			// enter into the directory
			file_manager_enter_into_folder(file_manager_get_item_name(selected_item));
			file_browser_update_shown_files();
		} else {
			// play file
			buttons_remove_key_event_callback();
			music_player_start(selected_item);
			return DIE;
		}
	} else if (received_key == KEY_CANCEL) {
		// If we're already on the root folder, then return to the global interface, otherwise return
		// to the above folder
		if (file_manager_is_at_root()) {
			file_browser_stop();
			return DIE;
		} else {
			file_manager_exit_from_folder();
			file_browser_update_shown_files();
		}
	}

	received_key = KEY_NONE;

	return WAIT_FOR_RESUME;
}

