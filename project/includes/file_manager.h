#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include "stdint.h"

#define MAX_PATH_LENGTH		256

int32_t file_manager_mount_disk(void);

int32_t file_manager_change_directory(char* path);
int32_t file_manager_enter_into_folder(char* dir_name);
int32_t file_manager_exit_from_folder();

char* file_manager_get_curr_path_pointer(void);
char* file_manager_get_current_folder_name(void);

uint8_t file_manager_is_at_root(void);
uint16_t file_manager_get_item_count(void);
uint8_t file_manager_is_item_a_dir(uint16_t index);
char* file_manager_get_item_name(uint16_t index);

#endif // _FILE_MANAGER_H_
