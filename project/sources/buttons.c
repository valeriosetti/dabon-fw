#include "buttons.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"
#include "kernel.h"
#include "systick.h"

#define debug_msg(format, ...)		debug_printf("[buttons] " format, ##__VA_ARGS__)

// Macros for buttons' polling
uint8_t is_VOL_UP_pressed()		{ return (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_0) == 0); }
uint8_t is_VOL_DOWN_pressed()	{ return (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_1) == 0); }
uint8_t is_OK_pressed()			{ return (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_4) == 0); }
uint8_t is_RIGHT_pressed()		{ return (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_5) == 0); }
uint8_t is_CANCEL_pressed()		{ return (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_0) == 0); }
uint8_t is_UP_pressed() 		{ return (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_12) == 0); }
uint8_t is_LEFT_pressed()    	{ return (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_13) == 0); }
uint8_t is_DOWN_pressed()    	{ return (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_14) == 0); }

void (*keypress_callback_func)(uint8_t, uint8_t);

typedef struct {
	uint8_t id;
	uint8_t status;
	uint32_t press_start_tick;
	uint8_t (*is_pressed_func)(void);
} BUTTON_STATE;

BUTTON_STATE buttons[] = {
	{ .id = KEY_UP, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_UP_pressed },
	{ .id = KEY_DOWN, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_DOWN_pressed },
	{ .id = KEY_LEFT, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_LEFT_pressed },
	{ .id = KEY_RIGHT, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_RIGHT_pressed },
	{ .id = KEY_OK, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_OK_pressed },
	{ .id = KEY_CANCEL, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_CANCEL_pressed },
	{ .id = KEY_VOL_UP, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_VOL_UP_pressed },
	{ .id = KEY_VOL_DOWN, .status = KEY_RELEASED, .press_start_tick = 0, .is_pressed_func = is_VOL_DOWN_pressed },
};

ALLOCATE_TASK(buttons, 10);
#define BUTTON_SCAN_INTERVAL		25
#define BUTTON_DEBOUNCE_INTERVAL	100

/*******************************************************************************/
/*	PUBLIC FUNCTIONS
/*******************************************************************************/
/*
 * Initialize the GPIOs
 */
void buttons_init()
{
    // Enable the GPIOs' peripheral clock
	RCC_SYSCFG_CLK_ENABLE();
	RCC_GPIOC_CLK_ENABLE();
	RCC_GPIOE_CLK_ENABLE();

    // PC0 --> VOL_UP
    // PC1 --> VOL_DOWN
    // PC4 --> OK
    // PC5 --> RIGHT
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODE0_Msk | GPIO_MODER_MODE1_Msk | GPIO_MODER_MODE4_Msk | GPIO_MODER_MODE5_Msk, 
                (MODER_INPUT << GPIO_MODER_MODE0_Pos) | (MODER_INPUT << GPIO_MODER_MODE1_Pos) | 
                (MODER_INPUT << GPIO_MODER_MODE4_Pos) | (MODER_INPUT << GPIO_MODER_MODE5_Pos));
	MODIFY_REG(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED0_Msk | GPIO_OSPEEDR_OSPEED1_Msk | GPIO_OSPEEDR_OSPEED4_Msk | GPIO_OSPEEDR_OSPEED5_Msk, 
                (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED0_Pos) | (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED1_Pos) | 
                (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED4_Pos) | (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED5_Pos));
	MODIFY_REG(GPIOC->PUPDR, GPIO_PUPDR_PUPD0_Msk | GPIO_PUPDR_PUPD1_Msk | GPIO_PUPDR_PUPD4_Msk | GPIO_PUPDR_PUPD5_Msk, 
                (PUPDR_PULL_UP << GPIO_PUPDR_PUPD0_Pos) | (PUPDR_PULL_UP << GPIO_PUPDR_PUPD1_Pos) |
                (PUPDR_PULL_UP << GPIO_PUPDR_PUPD4_Pos) | (PUPDR_PULL_UP << GPIO_PUPDR_PUPD5_Pos));
    
    // PE0 --> CANCEL
    // PE12 -> UP
    // PE13 -> LEFT
    // PE14 -> DOWN
    MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODE0_Msk | GPIO_MODER_MODE12_Msk | GPIO_MODER_MODE13_Msk | GPIO_MODER_MODE14_Msk, 
                (MODER_INPUT << GPIO_MODER_MODE0_Pos) | (MODER_INPUT << GPIO_MODER_MODE12_Pos) | 
                (MODER_INPUT << GPIO_MODER_MODE13_Pos) | (MODER_INPUT << GPIO_MODER_MODE14_Pos));
	MODIFY_REG(GPIOE->OSPEEDR, GPIO_OSPEEDR_OSPEED0_Msk | GPIO_OSPEEDR_OSPEED12_Msk | GPIO_OSPEEDR_OSPEED13_Msk | GPIO_OSPEEDR_OSPEED14_Msk, 
                (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED0_Pos) | (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED12_Pos) | 
                (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED13_Pos) | (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED14_Pos));
	MODIFY_REG(GPIOE->PUPDR, GPIO_PUPDR_PUPD0_Msk | GPIO_PUPDR_PUPD12_Msk | GPIO_PUPDR_PUPD13_Msk | GPIO_PUPDR_PUPD14_Msk, 
                (PUPDR_PULL_UP << GPIO_PUPDR_PUPD0_Pos) | (PUPDR_PULL_UP << GPIO_PUPDR_PUPD12_Pos) |
                (PUPDR_PULL_UP << GPIO_PUPDR_PUPD13_Pos) | (PUPDR_PULL_UP << GPIO_PUPDR_PUPD14_Pos));

	// Set the interrupt's source
	MODIFY_REG(SYSCFG->EXTICR[0], SYSCFG_EXTICR1_EXTI0_Msk | SYSCFG_EXTICR1_EXTI1_Msk,
				SYSCFG_EXTICR1_EXTI0_PE | SYSCFG_EXTICR1_EXTI1_PC);
	MODIFY_REG(SYSCFG->EXTICR[1], SYSCFG_EXTICR2_EXTI4_Msk | SYSCFG_EXTICR2_EXTI5_Msk,
				SYSCFG_EXTICR2_EXTI4_PC | SYSCFG_EXTICR2_EXTI5_PC);
	MODIFY_REG(SYSCFG->EXTICR[3], SYSCFG_EXTICR4_EXTI12_Msk | SYSCFG_EXTICR4_EXTI13_Msk | SYSCFG_EXTICR4_EXTI14_Msk,
				SYSCFG_EXTICR4_EXTI12_PE | SYSCFG_EXTICR4_EXTI13_PE | SYSCFG_EXTICR4_EXTI14_PE);

	// Configure interrupts (falling edges only)
	/*MODIFY_REG(EXTI->RTSR, EXTI_RTSR_TR0_Msk | EXTI_RTSR_TR1_Msk | EXTI_RTSR_TR4_Msk | EXTI_RTSR_TR5_Msk |
				EXTI_RTSR_TR12_Msk | EXTI_RTSR_TR13_Msk | EXTI_RTSR_TR14_Msk,
				EXTI_RTSR_TR0 | EXTI_RTSR_TR1 | EXTI_RTSR_TR4 | EXTI_RTSR_TR5 |
				EXTI_RTSR_TR12 | EXTI_RTSR_TR13 | EXTI_RTSR_TR14);*/
	MODIFY_REG(EXTI->FTSR, EXTI_FTSR_TR0_Msk | EXTI_FTSR_TR1_Msk | EXTI_FTSR_TR4_Msk | EXTI_FTSR_TR5_Msk |
				EXTI_FTSR_TR12_Msk | EXTI_FTSR_TR13_Msk | EXTI_FTSR_TR14_Msk,
				EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR4 | EXTI_FTSR_TR5 |
				EXTI_FTSR_TR12 | EXTI_FTSR_TR13 | EXTI_FTSR_TR14);
	MODIFY_REG(EXTI->IMR, EXTI_IMR_MR0_Msk | EXTI_IMR_MR1_Msk | EXTI_IMR_MR4_Msk | EXTI_IMR_MR5_Msk |
				EXTI_IMR_MR12_Msk | EXTI_IMR_MR13_Msk | EXTI_IMR_MR14_Msk,
				EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR4 | EXTI_IMR_MR5 |
				EXTI_IMR_MR12 | EXTI_IMR_MR13 | EXTI_IMR_MR14);

	// Enable interrupts
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// NOTE: the CANCEL button cannot be tied to any interrupt line in the current HW design
}

int32_t buttons_register_key_event_callback(void (*func)(uint8_t, uint8_t))
{
	keypress_callback_func = func;
	return 0;
}

int32_t buttons_remove_key_event_callback(void)
{
	keypress_callback_func = NULL;
	return 0;
}

/*******************************************************************************/
/*	TASK
/*******************************************************************************/
int32_t buttons_task_func()
{
	uint8_t is_any_button_pressed = FALSE;
	uint8_t index;

	// scan only the buttons that were pressed or on debouncing
	for (index=0; index<array_size(buttons); index++) {
		if (buttons[index].status == KEY_PRESSED_DEBOUNCING) {
			// Is it still pressed?
			if (buttons[index].is_pressed_func()) {
				is_any_button_pressed = TRUE;
				if (systick_get_tick_count() - buttons[index].press_start_tick > BUTTON_DEBOUNCE_INTERVAL) {
					// If the debounce time is expired then change state and generate the press event
					buttons[index].status = KEY_PRESSED;
					if (keypress_callback_func != NULL) {
						keypress_callback_func(index, KEY_PRESSED);
					}
				}
			} else {
				// the button is no more pressed
				buttons[index].status = KEY_RELEASED;
			}
		} else if (buttons[index].status == KEY_PRESSED) {
			if (!buttons[index].is_pressed_func()) {
				// if the button was previously pressed but has been released, then generate
				// the release event
				buttons[index].status = KEY_RELEASED;
				if (keypress_callback_func != NULL) {
					keypress_callback_func(index, KEY_RELEASED);
				}
			} else {
				// If the button is still pressed then just update the proper flag
				is_any_button_pressed = TRUE;
			}
		}
	}

	if (is_any_button_pressed) {
		return BUTTON_SCAN_INTERVAL;
	} else {
		return WAIT_FOR_RESUME;
	}
}

/*******************************************************************************/
/*	INTERRUPTS
/*******************************************************************************/
void EXTI0_IRQHandler()
{
	if (buttons[KEY_CANCEL].status == KEY_RELEASED) {
		buttons[KEY_CANCEL].status = KEY_PRESSED_DEBOUNCING;
		buttons[KEY_CANCEL].press_start_tick = systick_get_tick_count();
		kernel_activate_task_immediately(&buttons_task);
	}

	SET_BIT(EXTI->PR, EXTI_PR_PR0);
}

void EXTI1_IRQHandler()
{
	if (buttons[KEY_VOL_DOWN].status == KEY_RELEASED) {
		buttons[KEY_VOL_DOWN].status = KEY_PRESSED_DEBOUNCING;
		buttons[KEY_VOL_DOWN].press_start_tick = systick_get_tick_count();
		kernel_activate_task_immediately(&buttons_task);
	}

	SET_BIT(EXTI->PR, EXTI_PR_PR1);
}

void EXTI4_IRQHandler()
{
	if (buttons[KEY_OK].status == KEY_RELEASED) {
		buttons[KEY_OK].status = KEY_PRESSED_DEBOUNCING;
		buttons[KEY_OK].press_start_tick = systick_get_tick_count();
		kernel_activate_task_immediately(&buttons_task);
	}

	SET_BIT(EXTI->PR, EXTI_PR_PR4);
}

void EXTI9_5_IRQHandler()
{
	if (EXTI->PR & EXTI_PR_PR5) {	// RIGHT button
		if (buttons[KEY_RIGHT].status == KEY_RELEASED) {
			buttons[KEY_RIGHT].status = KEY_PRESSED_DEBOUNCING;
			buttons[KEY_RIGHT].press_start_tick = systick_get_tick_count();
			kernel_activate_task_immediately(&buttons_task);
		}

		SET_BIT(EXTI->PR, EXTI_PR_PR5);
	} else {
		debug_msg("Unknown interrupt source for EXTI9_5_IRQ. EXTI->PR=0x%x\n", EXTI->PR);
		MODIFY_REG(EXTI->PR, EXTI_PR_PR5 | EXTI_PR_PR6 | EXTI_PR_PR7 | EXTI_PR_PR8 | EXTI_PR_PR9, 0xFFFF);
	}
}

void EXTI15_10_IRQHandler()
{
	if (EXTI->PR & EXTI_PR_PR12) {	// UP button
		if (buttons[KEY_UP].status == KEY_RELEASED) {
			buttons[KEY_UP].status = KEY_PRESSED_DEBOUNCING;
			buttons[KEY_UP].press_start_tick = systick_get_tick_count();
			kernel_activate_task_immediately(&buttons_task);
		}

		SET_BIT(EXTI->PR, EXTI_PR_PR12);
	} else
	if (EXTI->PR & EXTI_PR_PR13) { // LEFT button
		if (buttons[KEY_LEFT].status == KEY_RELEASED) {
			buttons[KEY_LEFT].status = KEY_PRESSED_DEBOUNCING;
			buttons[KEY_LEFT].press_start_tick = systick_get_tick_count();
			kernel_activate_task_immediately(&buttons_task);
		}

		SET_BIT(EXTI->PR, EXTI_PR_PR13);
	} else
	if (EXTI->PR & EXTI_PR_PR14) { // DOWN button
		if (buttons[KEY_DOWN].status == KEY_RELEASED) {
			buttons[KEY_DOWN].status = KEY_PRESSED_DEBOUNCING;
			buttons[KEY_DOWN].press_start_tick = systick_get_tick_count();
			kernel_activate_task_immediately(&buttons_task);
		}

		SET_BIT(EXTI->PR, EXTI_PR_PR14);
	} else {
		debug_msg("Unknown interrupt source for EXTI15_10_IRQ. EXTI->PR=0x%x\n", EXTI->PR);
		MODIFY_REG(EXTI->PR, EXTI_PR_PR10 | EXTI_PR_PR11 | EXTI_PR_PR12 | EXTI_PR_PR13 | EXTI_PR_PR14 | EXTI_PR_PR15, 0xFFFF);
	}
}

/*******************************************************************************/
/*	SHELL COMMANDS
/*******************************************************************************/
/*
 * Scan the buttons
 */
int buttons_scan(int argc, char *argv[])
{
    if (is_VOL_UP_pressed()) 
        debug_msg("VOL_UP\n");
    if (is_VOL_DOWN_pressed())
        debug_msg("VOL_DOWN\n");
    if (is_RIGHT_pressed())
        debug_msg("RIGHT\n");
    if (is_DOWN_pressed())
        debug_msg("DOWN\n");
    if (is_CANCEL_pressed())
        debug_msg("CANCEL\n");
    if (is_UP_pressed()) 
        debug_msg("UP\n");
    if (is_LEFT_pressed())   
        debug_msg("LEFT\n");
    if (is_OK_pressed())
        debug_msg("OK\n");

    return 0;
}
