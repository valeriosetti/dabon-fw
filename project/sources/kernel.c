#include "kernel.h"
#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "i2c.h"
#include "uart.h"
#include "debug_printf.h"
#include "output_i2s.h"
#include "spi.h"
#include "timer.h"
#include "Si468x.h"
#include "fsmc.h"
#include "oled.h"
#include "sd_card.h"
#include "sd_card_detect.h"
#include "ff.h"
#include "systick.h"
#include "mp3dec.h"
#include "sgtl5000.h"
#include "shell.h"
#include "buttons.h"
#include "main_menu.h"

#define debug_msg(_format_, ...)	debug_printf("[Kernel] " _format_, ##__VA_ARGS__)

// Private variables
uint32_t tasks_count = 0;
struct TASK* active_tasks_list = NULL;  // list of active tasks (ordered based on priority)
struct TASK* dead_tasks_list = NULL;  // list of dead tasks (this is not ordered, of course)
struct TASK* active_task = NULL;  // pointer to the current active task (NULL if there's no active task)

// Private functions
static void kernel_add_task_to_list(struct TASK* input_task_ptr, struct TASK** task_list_ptr);
static struct TASK* kernel_get_next_task_to_run();
static void kernel_initialize_modules();

/********************************************************************/
/*	KERNEL - MODULES' RELATED FUNFCTION	*/
/********************************************************************/
static void kernel_initialize_modules()
{
	// Low level drivers
	uart_init();
	timer_init();
	i2c_init();
	spi_init();
	output_i2s_init();
	fsmc_init();
	SD_Init();
	systick_initialize();
	buttons_init();

	systick_wait_for_ms(50);

	// Peripherals
	eeprom_init();
	Si468x_init();
	sgtl5000_init();
	oled_init();

	// High level initializations
	main_menu_init();

	debug_msg("Initialization completed\n");
}

static void kernel_suspend_modules()
{
	
}

static void kernel_resume_modules()
{
	
}

/********************************************************************/
/*	KERNEL - PRIVATE FUNCTIONS	*/
/********************************************************************/
/*
 * Add a the task to the specified list
 */
static void kernel_add_task_to_list(struct TASK* input_task_ptr, struct TASK** task_list_ptr)
{
	// first task in the list
	if (*task_list_ptr == NULL) {
		*task_list_ptr = input_task_ptr;
	} else { // following elements
		// look for the right place on the list based on the task's priority
		struct TASK* curr_task_ptr = *task_list_ptr;
		struct TASK* prev_task_ptr = NULL;
		while ((curr_task_ptr != NULL) && ((input_task_ptr->priority)>(curr_task_ptr->priority))) {
			prev_task_ptr = curr_task_ptr;
			curr_task_ptr = curr_task_ptr->next_task;
		}
		
		if (prev_task_ptr == NULL) {	
			// replace the first element in the list
			input_task_ptr->next_task = curr_task_ptr;
			*task_list_ptr = input_task_ptr;
		} else if (curr_task_ptr == NULL) {
			// add to the end of the list
			prev_task_ptr->next_task = input_task_ptr;
		} else {
			// add the element in the middle of the list
			prev_task_ptr->next_task = input_task_ptr;
			input_task_ptr->next_task = curr_task_ptr;
		}
	}
	input_task_ptr->id = tasks_count;
	tasks_count ++;
}

/*
 * Remove the selected task from the specified list
 */
static int32_t kernel_remove_task_from_list(struct TASK* task_ptr, struct TASK** task_list_ptr)
{
	// If the list is empty there's no task which can be removed
	if (*task_list_ptr == NULL) {
		return -1;
	}
	
	struct TASK* prev_ptr = NULL;
	struct TASK* curr_ptr = *task_list_ptr;
	
	// Find the specified task
	while (curr_ptr != task_ptr) {
		if (curr_ptr->next_task != NULL) {
			// Move to the next element
			prev_ptr = curr_ptr;
			curr_ptr = curr_ptr->next_task;
		} else {
			// End of the list reached but the task was not found!
			return -1;
		}
	}
	
	if (prev_ptr == NULL) {	// The task is at the beginning of the list
		*task_list_ptr = curr_ptr->next_task;
	} else { // The task is in the middle or at the end of the list
		prev_ptr->next_task = curr_ptr->next_task;
	}
	
	return 0;
}

/*
 * Returns a pointer to the next active task which should be set on execution.
 * A NULL value is returned if there's no ready task to run.
 */
static struct TASK* kernel_get_next_task_to_run()
{
	uint32_t current_tick_count = systick_get_tick_count();
	struct TASK* curr_task_ptr = active_tasks_list;
	
	while (curr_task_ptr != NULL) {
		if (curr_task_ptr->status == TASK_STATE_SLEEPING) {
			if (current_tick_count >= curr_task_ptr->resume_at_tickcount)
				return curr_task_ptr;
		} 
		curr_task_ptr = curr_task_ptr->next_task;
	}
	return NULL;
}

/*
 * This is the first kernel function called after reset and it includes the scheduler.
 * The function is "naked" because we don't need any prologue/epilogue as we're never 
 * supposed to exit from this looping function.
 */
__attribute__((naked)) void kernel_main(void)
{
    int32_t task_ret_val;
    
    // Configure the main clock
	ClockConfig_SetMainClockAndPrescalers(); 
	// initialize all the modules
	kernel_initialize_modules();
	// Scheduler loop
	while (1) {
		active_task = kernel_get_next_task_to_run();
		if (active_task != NULL) {
            task_ret_val = (active_task->func)(NULL);
            if (task_ret_val >= 0) {
                active_task->resume_at_tickcount = systick_get_tick_count() + (int32_t)task_ret_val;
            } else if (task_ret_val == WAIT_FOR_RESUME) {
                active_task->status = TASK_STATE_WAITING_FOR_RESUME;
            } else {
                kernel_kill_task(active_task);
            }
			active_task = NULL;
		}
	}
}

/********************************************************************/
/*	KERNEL - PUBLIC FUNCTIONS	*/
/********************************************************************/
/*
 * Initialize the task's stack and add it to the tasks' list
 */
void kernel_init_task(struct TASK* task_ptr)
{
	kernel_add_task_to_list(task_ptr, &dead_tasks_list);
}

/*
 * The selected task will be enabled and scheduled after the desired amount of milliseconds
 */
void kernel_activate_task_after_ms(struct TASK* task_ptr, int32_t delay)
{
	// If the task was dead, then move it to the active tasks' list
	if (task_ptr->status == TASK_STATE_DEAD) {
		kernel_remove_task_from_list(task_ptr, &dead_tasks_list);
		kernel_add_task_to_list(task_ptr, &active_tasks_list);
	}

	if (delay >= 0) {
		task_ptr->status = TASK_STATE_SLEEPING;
		task_ptr->resume_at_tickcount = systick_get_tick_count() + delay;
	} else {
		task_ptr->status = TASK_STATE_WAITING_FOR_RESUME;
	}
}

/*
 * The selected task will be enabled and scheduled as soon as possible (based on its priority)
 */
void kernel_activate_task_immediately(struct TASK* task_ptr)
{
	kernel_activate_task_after_ms(task_ptr, 0UL);
}

/*
 * This simply kills a task
 */
void kernel_kill_task(struct TASK* task_ptr)
{
	task_ptr->status = TASK_STATE_DEAD;
	if (kernel_remove_task_from_list(task_ptr, &active_tasks_list) >= 0) {
		kernel_add_task_to_list(task_ptr, &dead_tasks_list);
	}
}

/*
 * Return the status of the selected task
 */
uint8_t kernel_get_task_status(struct TASK* task_ptr)
{
	return task_ptr->status;
}
