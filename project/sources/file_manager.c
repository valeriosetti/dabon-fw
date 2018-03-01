#include "file_manager.h"
#include "ff.h"
#include "kernel.h"
#include "debug_printf.h"
#include "string.h"

#define debug_msg(format, ...)		debug_printf("[file_manager] " format, ##__VA_ARGS__)

FATFS file_system;
DIR curr_dir;

char curr_path[MAX_PATH_LENGTH] = "";

#define MAX_ITEM_NAME_LENGTH		(8+1+3+1)	// 8 for filename; 1 for the dot; 3 for extension; 1 for string termination
typedef struct{
	char name[MAX_ITEM_NAME_LENGTH];
	uint8_t is_dir; 	// '1' means directory, '0' means file
} FOLDER_ITEM;

#define MAX_ITEMS_IN_FOLDER		1024
__attribute__((section (".ccmram"))) FOLDER_ITEM items_list[MAX_ITEMS_IN_FOLDER];
uint16_t items_count;

/**************************************************************************/
/*	CORE FUNCTIONS
/**************************************************************************/
/*
 * Mount the SD card and enter into the root folder
 */
int32_t file_manager_mount_disk()
{
	memset(curr_path, '\0', sizeof(curr_path));

	if (f_mount(&file_system, "", 0) != FR_OK) {
		debug_msg("Error mounting the sd card\n");
		return -1;
	}

	if (file_manager_enter_into_folder("/") < 0) {
		return -1;
	}

	return 0;
}

/*
 * Update the list of all the files included in the current folder
 */
static int32_t file_manager_list_files()
{
	FILINFO file_info;

	items_count = 0;

	do {
		if (f_readdir(&curr_dir, &file_info) != FR_OK) {
			debug_msg("Error while reading dir content\n");
			return -1;
		}
		// A null string is returned as file name when the last element is reached
		if (file_info.fname[0] != '\0') {
			strncpy(items_list[items_count].name, file_info.fname, MAX_ITEM_NAME_LENGTH);
			items_list[items_count].is_dir = (file_info.fattrib & AM_DIR) ? 1 : 0;
			items_count++;
		}
	} while (file_info.fname[0] != '\0');

	return 0;
}

/*
 * Change current working directory
 */
int32_t file_manager_change_directory(char* path)
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
int32_t file_manager_enter_into_folder(char* dir_name)
{
	uint16_t curr_path_len = strlen(curr_path);
	char* curr_path_ptr = &curr_path[curr_path_len];
	// If "curr_path" is not null and it doesn't end with a "/" then add it
	if ((curr_path_len != 0) && (curr_path_ptr[-1] != '/')) {
		*curr_path_ptr = '/';
		curr_path_ptr++;
	}
	strcpy(curr_path_ptr, dir_name);

	if (file_manager_change_directory(curr_path) < 0) {
		// Reset "curr_path" to the parent folder
		curr_path[curr_path_len] = '\0';
		return -1;
	}

	return file_manager_list_files();
}

/*
 * Modify the "curr_path" in order to return to the parent directory, then
 * try to move to that folder
 */
int32_t file_manager_exit_from_folder()
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
	if (file_manager_change_directory(curr_path) < 0) {
		return -2;
	}

	return file_manager_list_files();
}

/**************************************************************************/
/*	UTILITIES
/**************************************************************************/
/*
 * Returns a pointer to the "curr_path" string
 */
char* file_manager_get_curr_path_pointer()
{
	return curr_path;
}

/*
 * Returns a pointer inside the "curr_path" string at which
 * the name of the current folder starts
 */
char* file_manager_get_current_folder_name()
{
	uint16_t pos = strlen(curr_path)-1;
	while ((pos>0) && (curr_path[pos]!='/')) {
		pos--;
	}
	return &curr_path[pos];
}

/*
 * Returns the number of items in the current folder
 */
uint16_t file_manager_get_item_count()
{
	return items_count;
}

/*
 * Return a boolean which tells if the current folder is root or not
 */
uint8_t file_manager_is_at_root()
{
	return ( (strlen(curr_path)-1) == 0);
}

/*
 * Returns true/false depending if the requested item is a folder or not.
 * Note: if the index is not valid a FALSE is returned
 */
uint8_t file_manager_is_item_a_dir(uint16_t index)
{
	if (index > items_count) {
		return FALSE;
	}
	return (items_list[index].is_dir);
}

/*
 * Returns the name of the requested item
 * Note: if the index is not valid a null pointer is returned
 */
char* file_manager_get_item_name(uint16_t index)
{
	if (index > items_count) {
		return NULL;
	}
	return (items_list[index].name);
}
