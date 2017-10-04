#include "shell.h"
#include "debug_printf.h"
#include "string.h"
#include "Si468x.h"
#include "systick.h"
#include "utils.h"
#include "sgtl5000.h"
#include "buttons.h"
#include "eeprom.h"

#define debug_msg(...)		debug_printf_with_tag("[shell] ", __VA_ARGS__)

#define COMMAND_LINE_MAX_LENGTH     128
#define MAX_ARGS_NUM        5
uint8_t command_buffer[COMMAND_LINE_MAX_LENGTH];
uint16_t cmd_buff_pos = 0;
uint8_t cmd_complete_flag = 0;

static int shell_list_commands(int argc, char *argv[]);

typedef struct {
    char* cmd_name;
    int (*function)(int argc, char *argv[]);
} SINGLE_SHELL_CMD;

SINGLE_SHELL_CMD shell_cmd_list[] = {
    {"list_cmds", shell_list_commands},
    {"fm_tune", fm_tune},
    {"systick_gettime", systick_gettime},
    {"reset", reset},
    {"set_hp_out_volume", set_hp_out_volume},
    {"sgtl5000_dump_registers", sgtl5000_dump_registers},
    {"buttons_scan", buttons_scan},
    {"program_firmware", eeprom_program_firmware},
    {"show_partition_table", eeprom_show_partition_table},
	{}// do not remove this empty cell!!
};

/*
 * Process the command line
 */
static int shell_process_cmd_line(void)
{
	char *funct = NULL;
	char *args[MAX_ARGS_NUM];
	char *token;
	int argc = 0;
	char separator[2] = " ";

	// Parse all the elements of the command line
	token = strtok(command_buffer, separator);
	while(token != NULL){
		// The maximum number of elements was reached! Exit this function returning
		// and error
		if (argc == MAX_ARGS_NUM){
			return -1;
		}

		if (funct == NULL){
			// the first element of the line is the function
			funct = token;
		}else{
			// the other elements are the parameters
			args[argc] = token;
			argc++;
		}
		token = strtok(NULL, separator);
	};
	cmd_buff_pos = 0;
	
	// Search the selected function and call it with the specified parameters    
    SINGLE_SHELL_CMD* curr_cmd = shell_cmd_list;
    while (curr_cmd->cmd_name != NULL) {
        if(strcmp(curr_cmd->cmd_name, funct) == 0) {
            // If a matching was found, then call the function
            curr_cmd->function(argc, args);
            return 0;
        }
        curr_cmd++; // move to the next command
    }
	
    debug_msg("unknown command\n");
	return -2;
}

/*
 * Add a new char to the current command line
 */
void shell_add_char(uint8_t input_char)
{
    if ((input_char == '\n') || (input_char == '\r')) {
    	// Process the command
		cmd_complete_flag = 1;
		uart_put_char('\n');
		uart_put_char('\r');
    } else if (input_char == '\b') {
    	// Clear a char from the command line
    	if (cmd_buff_pos > 0) {
    		cmd_buff_pos--;
    		command_buffer[cmd_buff_pos] = '\0';
    		// echo the character back to the console
    		uart_put_char('\b');
    		uart_put_char(' ');
    		uart_put_char('\b');
    	}
    } else {
    	// Add the incoming char to the current cmd line
		if (cmd_buff_pos < (COMMAND_LINE_MAX_LENGTH-2)) {
			command_buffer[cmd_buff_pos] = input_char;
			cmd_buff_pos++;
			command_buffer[cmd_buff_pos] = '\0';
			// echo the character back to the console
			uart_put_char(input_char);
		}
    }
}

/*
 * This is called by the main loop. Its main purpose is to process the
 * command line once the new_line char is received
 */
void shell_run()
{
    if (cmd_complete_flag) {
        shell_process_cmd_line();
        cmd_complete_flag = 0;
    }
}

/*
 * Simple test function for testing shell commands
 */ 
static int shell_list_commands(int argc, char *argv[])
{
	SINGLE_SHELL_CMD* curr_cmd = shell_cmd_list;
	while (curr_cmd->cmd_name != NULL) {
		debug_printf("%s\n", curr_cmd->cmd_name);
		curr_cmd++; // move to the next command
	}
}
