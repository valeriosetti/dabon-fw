#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "stdint.h"

#define KEY_RELEASED			0
#define KEY_PRESSED_DEBOUNCING	1
#define KEY_PRESSED				2

#define KEY_UP			0
#define KEY_DOWN		1
#define KEY_LEFT		2
#define KEY_RIGHT		3
#define KEY_OK			4
#define KEY_CANCEL		5
#define KEY_VOL_UP		6
#define KEY_VOL_DOWN	7
#define KEY_NONE		0xFF

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
