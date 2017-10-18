#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "stdint.h"

#define KEY_PRESSED		0
#define KEY_RELEASED	1

#define KEY_NONE		0
#define KEY_UP			1
#define KEY_DOWN		2
#define KEY_LEFT		3
#define KEY_RIGHT		4
#define KEY_OK			5
#define KEY_CANCEL		6
#define KEY_VOL_UP		7
#define KEY_VOL_DOWN	8

// Public functions
void buttons_init(void);
int32_t buttons_register_key_event_callback(void (*func)(uint8_t, uint8_t));
int32_t buttons_remove_key_event_callback(void);

// Interrupts
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

// shell commands
int buttons_scan(int argc, char *argv[]);

#endif //_BUTTONS_H_
