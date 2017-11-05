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
char curr_path[MAX_PATH_LENGTH] = "";
#define MAX_ITEMS_IN_FOLDER		512
#define MAX_NAME_LENGTH		24

typedef struct{
	char name[MAX_NAME_LENGTH];
	uint8_t is_dir; 	// '1' means directory, '0' means file
} LIST_ITEM;

static struct {
	LIST_ITEM item[MAX_ITEMS_IN_FOLDER];
	uint16_t items_count;
	int16_t first_shown_item;
	int16_t selected_item;
} menu_items;


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

	menu_items.items_count = 0;

	do {
		if (f_readdir(&curr_dir, &file_info) != FR_OK) {
			debug_msg("Error while reading dir content\n");
			return -1;
		}
		// A null string is returned as file name when the last element is reached
		if (file_info.fname[0] != '\0') {
			strncpy(menu_items.item[menu_items.items_count].name, file_info.fname, MAX_NAME_LENGTH);
			menu_items.item[menu_items.items_count].is_dir = (file_info.fattrib & AM_DIR) ? 1 : 0;
			menu_items.items_count++;
		}
	} while (file_info.fname[0] != '\0');

	if (menu_items.items_count > 0) {
		menu_items.first_shown_item = 0;
		menu_items.selected_item = 0;
	} else {
		menu_items.first_shown_item = -1;
		menu_items.selected_item = -1;
	}

	return 0;
}

/*
 * Change current working directory
 */
static int32_t file_browser_change_directory(char* path)
{
	debug_msg("change folder: %s\n", path);

	if (f_opendir(&curr_dir, curr_path) != FR_OK) {
		debug_msg("Unable to change directory\n");
		return -1;
	}
	return 0;
}

/*
 * Modify the "curr_path" in order to include the new folder, then
 * try to move to that folder
 */
static int32_t file_browser_enter_into_folder(char* dir_name)
{
	uint16_t curr_path_len = strlen(curr_path);
	char* curr_path_ptr = &curr_path[curr_path_len];
	// If "curr_path" is not null and it doesn't end with a "/" then add it
	if ((curr_path_len != 0) && (curr_path_ptr[-1] != '/')) {
		*curr_path_ptr = '/';
		curr_path_ptr++;
	}
	strcpy(curr_path_ptr, dir_name);

	if (file_browser_change_directory(curr_path) < 0) {
		// Reset "curr_path" to the parent folder
		curr_path[curr_path_len] = '\0';
		return -1;
	}

	return file_browser_list_files();
}

/*
 * Modify the "curr_path" in order to return to the parent directory, then
 * try to move to that folder
 */
static int32_t file_browser_exit_from_folder()
{
	uint16_t pos=0;
	// Go to the end of the string
	pos = strlen(curr_path);
	// If we're already in the root folder it doesn't make any sense to try
	// to go the parent folder
	if (pos<=1) {
		return -1;
	}
	// Now go backward until a '/' char is found
	pos--;
	while ((pos>0) && (curr_path[pos]!='/')) {
		pos--;
	}
	// replace '/' with '\0'
	if (pos>0) {
		curr_path[pos] = '\0';
	}else {
		curr_path[pos+1] = '\0';
	}
	if (file_browser_change_directory(curr_path) < 0) {
		return -2;
	}

	return file_browser_list_files();
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
	uint16_t pos = strlen(curr_path)-1;
	if (pos == 0) {
		oled_print_text_at_xy("Root folder", FONT_CHAR_WIDTH, TITLE_LINE);
	} else {
		while ((pos>0) && (curr_path[pos]!='/')) {
			pos--;
		}
		oled_print_text_at_xy(&curr_path[pos+1], FONT_CHAR_WIDTH, TITLE_LINE);
	}
	oled_invert_line_color(0);

	uint16_t offset_index = 0;
	while (((menu_items.first_shown_item+offset_index)<menu_items.items_count) && (offset_index<LAST_LIST_LINE)) {
		// Draw a marker before the selected item
		if (menu_items.first_shown_item+offset_index == menu_items.selected_item) {
			oled_print_text_at_xy(">", 0, offset_index+1);
		}
		// If it's a folder then put a "[]" symbol before the name
		if (menu_items.item[menu_items.selected_item].is_dir) {
			oled_print_text_at_xy("[] ", FONT_CHAR_WIDTH, offset_index+1);
			oled_print_text_at_xy(menu_items.item[menu_items.first_shown_item+offset_index].name, FONT_CHAR_WIDTH*4, offset_index+1);
		} else {
			oled_print_text_at_xy(menu_items.item[menu_items.first_shown_item+offset_index].name, FONT_CHAR_WIDTH, offset_index+1);
		}
		offset_index++;
	}

	// add up and down arrow on the right part of the screen
	if (menu_items.selected_item < menu_items.items_count-1) {
		oled_print_char_at_xy(DOWN_ARROW, 24*FONT_CHAR_WIDTH, LAST_LIST_LINE);
	}
	if (menu_items.selected_item > 0) {
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

	memset(curr_path, '\0', sizeof(curr_path));

	if (f_mount(&file_system, "", 0) != FR_OK) {
		debug_msg("Error mounting the sd card\n");
		file_browser_stop();
		return;
	}

	if (file_browser_enter_into_folder("/") < 0) {
		file_browser_stop();
		return;
	}

	file_browser_update_shown_files();
	received_key = KEY_NONE;
}

/*
 * Main task
 */
int32_t file_browser_task_func(void* arg)
{
	// Process key
	if (received_key == KEY_DOWN) {
		if ((menu_items.selected_item<menu_items.items_count-1)) {
			menu_items.selected_item += 1;
		}
		if ((menu_items.selected_item-menu_items.first_shown_item)>=MAX_SHOWN_ITEMS) {
			menu_items.first_shown_item = menu_items.selected_item-MAX_SHOWN_ITEMS+1;
		}
		file_browser_update_shown_files();
	} else if (received_key == KEY_UP) {
		if (menu_items.selected_item>0) {
			menu_items.selected_item -= 1;
		}
		if (menu_items.selected_item<menu_items.first_shown_item) {
			menu_items.first_shown_item = menu_items.selected_item;
		}
		file_browser_update_shown_files();
	} else if (received_key == KEY_OK) {
		if (menu_items.item[menu_items.selected_item].is_dir) {
			// enter into the directory
			file_browser_enter_into_folder(menu_items.item[menu_items.selected_item].name);
			file_browser_update_shown_files();
		} else {
			// play file
			debug_msg("this file should be played\n");
		}
	} else if (received_key == KEY_CANCEL) {
		// If we're already on the root folder, then return to the global interface, otherwise return
		// to the above folder
		if (strcmp(curr_path, "/") == 0) {
			file_browser_stop();
			return DIE;
		} else {
			file_browser_exit_from_folder();
			file_browser_update_shown_files();
		}
	}

	received_key = KEY_NONE;

	return WAIT_FOR_RESUME;
}

