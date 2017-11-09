#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "stdint.h"
#include "utils.h"

// Allowed task states
#define TASK_STATE_DEAD 					0x00
#define TASK_STATE_RUNNING 					0x01
#define TASK_STATE_SLEEPING					0x02
#define TASK_STATE_WAITING_FOR_RESUME		0x04

// Task return options (other than "real" sleeping values)
#define IMMEDIATELY     	((int32_t)0)
#define WAIT_FOR_RESUME     ((int32_t)-1)
#define DIE		            ((int32_t)-2)

// Global interrupt control macros
#define kernel_disable_configurable_interrupts()		//__asm("cpsid i")
#define kernel_enable_configurable_interrupts()			//__asm("cpsie i")
#define kernel_disable_all_interrupts()					//__asm("cpsid f")
#define kernel_enable_all_interrupts()					//__asm("cpsie f")

// Task structure
struct TASK {
	uint8_t status;		// status of the task
	uint8_t priority;
	uint16_t id;
	char* name;
	int32_t (*func)(void);
	uint32_t resume_at_tickcount;
	struct TASK* next_task;
};

#define ALLOCATE_TASK(_name_, _priority_)	\
    int32_t _name_##_task_func(); \
	struct TASK _name_##_task = {	\
		.priority = _priority_, \
		.resume_at_tickcount = 0,	\
		.name = #_name_, \
		.func = _name_##_task_func, \
		.next_task = NULL,	\
		.status = TASK_STATE_DEAD,	\
	};

// Core functions
void kernel_main(void);

// General purpose functions
void kernel_init_task(struct TASK* task_ptr);
void kernel_activate_task_after_ms(struct TASK* task, int32_t delay);
void kernel_activate_task_immediately(struct TASK* task);
uint8_t kernel_get_task_status(struct TASK* task_ptr);
void kernel_kill_task(struct TASK* task_ptr);

#endif // _KERNEL_H_
