#include "kernel.h"
#include "debug_printf.h"
#include "buttons.h"
#include "oled.h"
#include "file_browser.h"
#include "main_menu.h"
#include "ff.h"
#include "string.h"

#define debug_msg(format, ...)		debug_printf("[file_browser] " format, ##__VA_ARGS__)

ALLOCATE_TASK(file_browser, 200);

uint8_t received_key;

FATFS file_system;
DIR curr_dir;
#define MAX_PATH_LENGTH		256
char curr_path[MAX_PATH_LENGTH] = "/";
#define MAX_FILES_IN_FOLDER		512
#define MAX_FILENAME_LENGTH		32
char file_names[MAX_FILES_IN_FOLDER][MAX_FILENAME_LENGTH];
uint16_t files_count;
uint16_t first_file_shown;

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
 * Update the list of all the files included in the current folder
 */
static int32_t file_browser_list_files()
{
	FILINFO file_info;

	do {
		if (f_readdir(&curr_dir, &file_info) != FR_OK) {
			debug_msg("Error while reading dir content\n");
			return -1;
		}
		// A null string is returned as file name when the last element is reached
		if (file_info.fname[0] != '\0') {
			strncpy(file_names[files_count], file_info.fname, MAX_FILENAME_LENGTH);
			files_count++;
		}
	} while (file_info.fname[0] != '\0');

	return 0;
}

/*
 * Update the shown list of files
 */
void file_browser_update_shown_files()
{
	oled_clear_display();

	if (files_count == 0) {
		oled_print_text_at_xy("<no files>", 0, 0);
		return;
	}

	uint16_t curr_line = 0;
	while (((first_file_shown+curr_line)<files_count) || (curr_line<OLED_MAX_NUMBER_OF_TEXT_LINES)) {
		oled_print_text_at_xy(file_names[first_file_shown+curr_line], 0, curr_line);
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

	if (f_mount(&file_system, "", 0) != FR_OK) {
		debug_msg("Error mounting the sd card\n");
		file_browser_stop();
		return;
	}
	files_count = 0;
	first_file_shown = 0;
	if (f_opendir(&curr_dir, curr_path) != FR_OK) {
		debug_msg("Unable to open root dir\n");
		file_browser_stop();
		return;
	}

	if (file_browser_list_files() < 0) {
		file_browser_stop();
		return;
	}
}

/*
 * Main task
 */
int32_t file_browser_task_func(void* arg)
{
	// Process key
	if ((received_key == KEY_UP) || (received_key == KEY_DOWN)) {

		file_browser_update_shown_files();
	} else if (received_key == KEY_OK) {

	} else if (received_key == KEY_CANCEL) {
		file_browser_stop();
		return DIE;
	}

	return WAIT_FOR_RESUME;
}

