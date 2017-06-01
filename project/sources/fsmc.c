#include "fsmc.h"
#include "stm32f407xx.h"
#include "gpio.h"
#include "utils.h"
#include "clock_configuration.h"

/*
 * Initialize the FSMC controller and GPIOs
 */
void fsmc_init()
{
	// Enable the GPIOs' peripheral clock
	RCC_GPIOD_CLK_ENABLE();
	RCC_GPIOE_CLK_ENABLE();

	// The following pins will be used from the FSMC controller
	// 	- LCD_D0 -> PD14 (AF12)
	// 	- LCD_D1 -> PD15 (AF12)
	// 	- LCD_D2 -> PD0 (AF12)
	// 	- LCD_D3 -> PD1 (AF12)
	// 	- LCD_D4 -> PE7 (AF12)
	// 	- LCD_D5 -> PE8 (AF12)
	// 	- LCD_D6 -> PE9 (AF12)
	// 	- LCD_D7 -> PE10 (AF12)
	// 	- LCD_RD -> PD4 (AF12)
	// 	- LCD_R/W -> PD5 (AF12)
	// 	- LCD_D/C -> PD11 (AF12)
	// 	- LCD_CS -> PD7 (AF12)
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE14_Msk + GPIO_MODER_MODE15_Msk + GPIO_MODER_MODE0_Msk +
			GPIO_MODER_MODE1_Msk + GPIO_MODER_MODE4_Msk + GPIO_MODER_MODE5_Msk + GPIO_MODER_MODE11_Msk +
			GPIO_MODER_MODE7_Msk,
			(MODER_ALTERNATE << GPIO_MODER_MODE14_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE15_Pos) +
			(MODER_ALTERNATE << GPIO_MODER_MODE0_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE1_Pos) +
			(MODER_ALTERNATE << GPIO_MODER_MODE4_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE5_Pos) +
			(MODER_ALTERNATE << GPIO_MODER_MODE11_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE7_Pos) );
	MODIFY_REG(GPIOD->AFR[0], GPIO_AFRL_AFSEL0_Msk + GPIO_AFRL_AFSEL1_Msk + GPIO_AFRL_AFSEL4_Msk + GPIO_AFRL_AFSEL5_Msk +
			GPIO_AFRL_AFSEL7_Msk, (12UL << GPIO_AFRL_AFSEL0_Pos) + (12UL << GPIO_AFRL_AFSEL1_Pos) + (12UL << GPIO_AFRL_AFSEL4_Pos) +
			(12UL << GPIO_AFRL_AFSEL5_Pos) + (12UL << GPIO_AFRL_AFSEL7_Pos));
	MODIFY_REG(GPIOD->AFR[1], GPIO_AFRH_AFSEL14_Msk + GPIO_AFRH_AFSEL15_Msk + GPIO_AFRH_AFSEL11_Msk,
				(12UL << GPIO_AFRH_AFSEL14_Pos) + (12UL << GPIO_AFRH_AFSEL15_Pos) + (12UL << GPIO_AFRH_AFSEL11_Pos));
	MODIFY_REG(GPIOD->OSPEEDR, GPIO_OSPEEDR_OSPEED14_Msk + GPIO_OSPEEDR_OSPEED15_Msk + GPIO_OSPEEDR_OSPEED0_Msk +
			GPIO_OSPEEDR_OSPEED1_Msk + GPIO_OSPEEDR_OSPEED4_Msk + GPIO_OSPEEDR_OSPEED5_Msk + GPIO_OSPEEDR_OSPEED11_Msk +
			GPIO_OSPEEDR_OSPEED7_Msk, (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED14_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED15_Pos) +
			(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED0_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED1_Pos) +
			(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED4_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED5_Pos) +
			(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED11_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED7_Pos));
	MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODE7_Msk + GPIO_MODER_MODE8_Msk + GPIO_MODER_MODE9_Msk + GPIO_MODER_MODE10_Msk,
				(MODER_ALTERNATE << GPIO_MODER_MODE7_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE8_Pos) +
				(MODER_ALTERNATE << GPIO_MODER_MODE9_Pos) + (MODER_ALTERNATE << GPIO_MODER_MODE10_Pos) );
	MODIFY_REG(GPIOE->AFR[0], GPIO_AFRL_AFSEL7_Msk, (12UL << GPIO_AFRL_AFSEL7_Pos) );
	MODIFY_REG(GPIOE->AFR[1], GPIO_AFRH_AFSEL8_Msk + GPIO_AFRH_AFSEL9_Msk + GPIO_AFRH_AFSEL10_Msk,
			(12UL << GPIO_AFRH_AFSEL8_Pos) + (12UL << GPIO_AFRH_AFSEL9_Pos) + (12UL << GPIO_AFRH_AFSEL10_Pos) );
	MODIFY_REG(GPIOE->OSPEEDR, GPIO_OSPEEDR_OSPEED7_Msk + GPIO_OSPEEDR_OSPEED8_Msk + GPIO_OSPEEDR_OSPEED9_Msk +
			GPIO_OSPEEDR_OSPEED10_Msk, (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED7_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED8_Pos) +
				(OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED9_Pos) + (OSPEEDR_50MHZ << GPIO_OSPEEDR_OSPEED10_Pos) );

	// Initialize the FSMC peripheral:
	//	- FSMC bank 1 (NOR memory; base address 0x6000 0000)
	//	- NOR bank 1 (using NE1 for memory selection)
	RCC_FSMC_CLK_ENABLE();
	SET_BIT(FSMC_Bank1->BTCR[0], FSMC_BCR1_MBKEN);	// Enable bank
	CLEAR_BIT(FSMC_Bank1->BTCR[0], FSMC_BCR1_MUXEN);	// No address/data multiplexing on the bus
	MODIFY_REG(FSMC_Bank1->BTCR[0], FSMC_BCR1_MTYP_Msk, (10UL << FSMC_BCR1_MTYP_Pos));	// NOR memory
	MODIFY_REG(FSMC_Bank1->BTCR[0], FSMC_BCR1_MWID_Msk, (0UL << FSMC_BCR1_MWID_Pos));	// 8bit bus width
	SET_BIT(FSMC_Bank1->BTCR[0], FSMC_BCR1_WREN);	// Write is disabled
	CLEAR_BIT(FSMC_Bank1->BTCR[0], FSMC_BCR1_WAITEN);	// NWAIT signal is disabled
	SET_BIT(FSMC_Bank1->BTCR[0], FSMC_BCR1_EXTMOD);	// Extended mode
	MODIFY_REG(FSMC_Bank1->BTCR[1], FSMC_BTR1_ACCMOD_Msk, (10UL << FSMC_BTR1_ACCMOD_Pos));	// access mode C
	MODIFY_REG(FSMC_Bank1->BTCR[1], FSMC_BTR1_ADDSET_Msk, (4UL << FSMC_BTR1_ADDSET_Pos));	// ADDSET ~ 4*6ns = 24ns
	MODIFY_REG(FSMC_Bank1->BTCR[1], FSMC_BTR1_DATAST_Msk, (31UL << FSMC_BTR1_DATAST_Pos));	// DATAST ~ 31*6ns = 186ns
	MODIFY_REG(FSMC_Bank1->BTCR[1], FSMC_BTR1_BUSTURN_Msk, (15UL << FSMC_BTR1_BUSTURN_Pos));	// BUSTURN ~ 15*6ns = 90ns
	MODIFY_REG(FSMC_Bank1E->BWTR[0], FSMC_BWTR1_ACCMOD_Msk, (10UL << FSMC_BWTR1_ACCMOD_Pos));	// access mode C
	MODIFY_REG(FSMC_Bank1E->BWTR[0], FSMC_BWTR1_ADDSET_Msk, (4UL << FSMC_BWTR1_ADDSET_Pos));	// ADDSET ~ 4*6ns = 24ns
	MODIFY_REG(FSMC_Bank1E->BWTR[0], FSMC_BWTR1_DATAST_Msk, (31UL << FSMC_BWTR1_DATAST_Pos));	// DATAST ~ 31*6ns = 186ns
	MODIFY_REG(FSMC_Bank1E->BWTR[0], FSMC_BWTR1_BUSTURN_Msk, (15UL << FSMC_BWTR1_BUSTURN_Pos));	// BUSTURN ~ 15*6ns = 90ns
}

