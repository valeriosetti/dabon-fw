#include "buttons.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"
#include "debug_printf.h"

#define debug_msg(...)		debug_printf_with_tag("[buttons] ", __VA_ARGS__)

// Macros for buttons' polling
#define is_VOL_UP_pressed()    (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_0) == 0)
#define is_VOL_DOWN_pressed()    (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_1) == 0)
#define is_OK_pressed()    (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_4) == 0)
#define is_RIGHT_pressed()    (READ_BIT(GPIOC->IDR, GPIO_IDR_IDR_5) == 0)
#define is_CANCEL_pressed()    (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_0) == 0)
#define is_UP_pressed()    (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_12) == 0)
#define is_LEFT_pressed()    (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_13) == 0)
#define is_DOWN_pressed()    (READ_BIT(GPIOE->IDR, GPIO_IDR_IDR_14) == 0)

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

	// Configure interrupts (both rising and falling edges)
	MODIFY_REG(EXTI->RTSR, EXTI_RTSR_TR0_Msk | EXTI_RTSR_TR1_Msk | EXTI_RTSR_TR4_Msk | EXTI_RTSR_TR5_Msk |
				EXTI_RTSR_TR12_Msk | EXTI_RTSR_TR13_Msk | EXTI_RTSR_TR14_Msk,
				EXTI_RTSR_TR0 | EXTI_RTSR_TR1 | EXTI_RTSR_TR4 | EXTI_RTSR_TR5 |
				EXTI_RTSR_TR12 | EXTI_RTSR_TR13 | EXTI_RTSR_TR14);
	MODIFY_REG(EXTI->FTSR, EXTI_FTSR_TR0_Msk | EXTI_FTSR_TR1_Msk | EXTI_FTSR_TR4_Msk | EXTI_FTSR_TR5_Msk |
				EXTI_FTSR_TR12_Msk | EXTI_FTSR_TR13_Msk | EXTI_FTSR_TR14_Msk,
				EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR4 | EXTI_FTSR_TR5 |
				EXTI_FTSR_TR12 | EXTI_FTSR_TR13 | EXTI_FTSR_TR14);
	MODIFY_REG(EXTI->IMR, EXTI_IMR_MR0_Msk | EXTI_IMR_MR1_Msk | EXTI_IMR_MR4_Msk | EXTI_IMR_MR5_Msk |
				EXTI_IMR_MR12_Msk | EXTI_IMR_MR13_Msk | EXTI_IMR_MR14_Msk,
				EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR4 | EXTI_IMR_MR5 |
				EXTI_IMR_MR12 | EXTI_IMR_MR13 | EXTI_IMR_MR14);

	MODIFY_REG(SYSCFG->EXTICR[0], SYSCFG_EXTICR1_EXTI0_Msk | SYSCFG_EXTICR1_EXTI1_Msk,
				SYSCFG_EXTICR1_EXTI0_PC | SYSCFG_EXTICR1_EXTI1_PC);
	MODIFY_REG(SYSCFG->EXTICR[1], SYSCFG_EXTICR2_EXTI4_Msk | SYSCFG_EXTICR2_EXTI5_Msk,
				SYSCFG_EXTICR2_EXTI4_PC | SYSCFG_EXTICR2_EXTI5_PC);
	MODIFY_REG(SYSCFG->EXTICR[3], SYSCFG_EXTICR4_EXTI12_Msk | SYSCFG_EXTICR4_EXTI13_Msk | SYSCFG_EXTICR4_EXTI14_Msk,
				SYSCFG_EXTICR4_EXTI12_PE | SYSCFG_EXTICR4_EXTI13_PE | SYSCFG_EXTICR4_EXTI14_PE);

	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// NOTE: the CANCEL button cannot be tied to any interrupt line in the current HW design
}

void EXTI0_IRQHandler()
{
	if (is_VOL_UP_pressed())
		debug_msg("VOL_UP press\n");
	else
		debug_msg("VOL_UP released\n");

	SET_BIT(EXTI->PR, EXTI_PR_PR0);
}

void EXTI1_IRQHandler()
{
	if (is_VOL_DOWN_pressed())
		debug_msg("VOL_DOWN press\n");
	else
		debug_msg("VOL_DOWN released\n");

	SET_BIT(EXTI->PR, EXTI_PR_PR1);
}

void EXTI4_IRQHandler()
{
	if (is_OK_pressed())
		debug_msg("OK press\n");
	else
		debug_msg("OK released\n");

	SET_BIT(EXTI->PR, EXTI_PR_PR4);
}

void EXTI9_5_IRQHandler()
{
	if (EXTI->PR & EXTI_PR_PR5) {	// RIGHT button
		if (is_RIGHT_pressed())
			debug_msg("RIGHT press\n");
		else
			debug_msg("RIGHT released\n");
		SET_BIT(EXTI->PR, EXTI_PR_PR5);
	} else {
		debug_msg("Unknown interrupt source for EXTI9_5_IRQ\n");
		MODIFY_REG(EXTI->PR, EXTI_PR_PR5 | EXTI_PR_PR6 | EXTI_PR_PR7 | EXTI_PR_PR8 | EXTI_PR_PR9, 0xFFFF);
	}
}

void EXTI15_10_IRQHandler()
{
	if (EXTI->PR & EXTI_PR_PR12) {	// UP button
		if (is_UP_pressed())
			debug_msg("UP press\n");
		else
			debug_msg("UP released\n");
		SET_BIT(EXTI->PR, EXTI_PR_PR12);
	} else
	if (EXTI->PR & EXTI_PR_PR13) { // LEFT button
		if (is_LEFT_pressed())
			debug_msg("LEFT press\n");
		else
			debug_msg("LEFT released\n");
		SET_BIT(EXTI->PR, EXTI_PR_PR13);
	} else
	if (EXTI->PR & EXTI_PR_PR14) { // DOWN button
		if (is_DOWN_pressed())
			debug_msg("DOWN press\n");
		else
			debug_msg("DOWN released\n");
		SET_BIT(EXTI->PR, EXTI_PR_PR14);
	} else {
		debug_msg("Unknown interrupt source for EXTI15_10_IRQ\n");
		MODIFY_REG(EXTI->PR, EXTI_PR_PR10 | EXTI_PR_PR11 | EXTI_PR_PR12 | EXTI_PR_PR13 | EXTI_PR_PR14 | EXTI_PR_PR15, 0xFFFF);
	}
}

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
