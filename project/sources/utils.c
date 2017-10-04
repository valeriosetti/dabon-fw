#include "utils.h"
#include "stm32f407xx.h"
#include "core_cm4.h"

/*
 * Perform a software reset of the STM32
 */
int reset(int argc, char *argv[])
{
	NVIC_SystemReset();
}
