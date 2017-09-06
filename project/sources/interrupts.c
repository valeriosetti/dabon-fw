#include "stdint.h"
#include "systick.h"
#include "sd_card.h"

// The following symbols are defined in the linker script
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;

extern uint32_t _siccmram;
extern uint32_t _sccmram;
extern uint32_t _eccmram;

extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

extern void main(void);

/*
 *  This is the default handler for all non-defined interrupts
 */
void default_handler(void)
{
	while (1);
}

__attribute__((naked)) void Reset_Handler(void)
{
	register uint32_t regMainStackPointer asm("sp") = (uint32_t)&_estack;
  
	/* Copy the data segment initializers from flash to SRAM */
	uint32_t* idata_begin = &_sidata;
	uint32_t* data_begin = &_sdata;
	uint32_t* data_end = &_edata;
	while (data_begin < data_end) *data_begin++ = *idata_begin++;

	/* Copy the data segment initializers from flash to CCRAM */
	/*uint32_t* cc_idata_begin = &_siccmram;
	uint32_t* cc_data_begin = &_sccmram;
	uint32_t* cc_data_end = &_eccmram;
	while (cc_data_begin < cc_data_end) *cc_data_begin++ = *cc_idata_begin++;*/
	
	/* Zero fill the bss segment. */
	uint32_t *bss_begin = &_sbss;
	uint32_t *bss_end = &_ebss;
	while (bss_begin < bss_end) *bss_begin++ = 0;

	main();
}

void NMI_Handler(void) __attribute((weak, alias("default_handler")));
void hardfault_handler(void) __attribute((weak, alias("default_handler")));
void HardFault_Handler(void) __attribute((weak, alias("default_handler")));
void MemManage_Handler(void) __attribute((weak, alias("default_handler")));
void BusFault_Handler(void) __attribute((weak, alias("default_handler")));
void UsageFault_Handler(void) __attribute((weak, alias("default_handler")));
void SVC_Handler(void) __attribute((weak, alias("default_handler")));
void DebugMon_Handler(void) __attribute((weak, alias("default_handler")));
void PendSV_Handler(void) __attribute((weak, alias("default_handler")));
//void SysTick_Handler(void) __attribute((weak, alias("default_handler")));
void WWDG_IRQHandler(void) __attribute((weak, alias("default_handler")));
void PVD_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TAMP_STAMP_IRQHandler(void) __attribute((weak, alias("default_handler")));
void RTC_WKUP_IRQHandler(void) __attribute((weak, alias("default_handler")));
void FLASH_IRQHandler(void) __attribute((weak, alias("default_handler")));
void RCC_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI0_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI2_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI4_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream0_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream2_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream4_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream5_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream6_IRQHandler(void) __attribute((weak, alias("default_handler")));
void ADC_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN1_TX_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN1_RX0_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN1_RX1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN1_SCE_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI9_5_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM1_BRK_TIM9_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM1_UP_TIM10_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM1_CC_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM2_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM4_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C1_EV_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C1_ER_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C2_EV_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C2_ER_IRQHandler(void) __attribute((weak, alias("default_handler")));
void SPI1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void SPI2_IRQHandler(void) __attribute((weak, alias("default_handler")));
void USART1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void USART2_IRQHandler(void) __attribute((weak, alias("default_handler")));
void USART3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void EXTI15_10_IRQHandler(void) __attribute((weak, alias("default_handler")));
void RTC_Alarm_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_FS_WKUP_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM8_BRK_TIM12_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM8_UP_TIM13_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM8_TRG_COM_TIM14_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM8_CC_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA1_Stream7_IRQHandler(void) __attribute((weak, alias("default_handler")));
void FSMC_IRQHandler(void) __attribute((weak, alias("default_handler")));
//void SDIO_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM5_IRQHandler(void) __attribute((weak, alias("default_handler")));
void SPI3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void UART4_IRQHandler(void) __attribute((weak, alias("default_handler")));
void UART5_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM6_DAC_IRQHandler(void) __attribute((weak, alias("default_handler")));
void TIM7_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream0_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream2_IRQHandler(void) __attribute((weak, alias("default_handler")));
//void DMA2_Stream3_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream4_IRQHandler(void) __attribute((weak, alias("default_handler")));
void ETH_IRQHandler(void) __attribute((weak, alias("default_handler")));
void ETH_WKUP_IRQHandler(void) __attribute((weak, alias("default_handler")));
void ETH_WKUP_IRQHandle(void) __attribute((weak, alias("default_handler")));
void CAN2_TX_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN2_RX0_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN2_RX1_IRQHandler(void) __attribute((weak, alias("default_handler")));
void CAN2_SCE_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_FS_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream5_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream6_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DMA2_Stream7_IRQHandler(void) __attribute((weak, alias("default_handler")));
void USART6_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C3_EV_IRQHandler(void) __attribute((weak, alias("default_handler")));
void I2C3_ER_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_HS_EP1_OUT_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_HS_EP1_IN_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_HS_WKUP_IRQHandler(void) __attribute((weak, alias("default_handler")));
void OTG_HS_IRQHandler(void) __attribute((weak, alias("default_handler")));
void DCMI_IRQHandler(void) __attribute((weak, alias("default_handler")));
void HASH_RNG_IRQHandler(void) __attribute((weak, alias("default_handler")));
void FPU_IRQHandler(void) __attribute((weak, alias("default_handler")));

__attribute((section(".isr_vector")))
uint32_t *isr_vectors[] = {
	(uint32_t *) &_estack,			
	(uint32_t *) Reset_Handler,
	(uint32_t *) NMI_Handler,
	(uint32_t *) HardFault_Handler,
	(uint32_t *) MemManage_Handler,
	(uint32_t *) BusFault_Handler,
	(uint32_t *) UsageFault_Handler,
	0,
	0,
	0,
	0,
	(uint32_t *) SVC_Handler,
	(uint32_t *) DebugMon_Handler,
	0,
	(uint32_t *) PendSV_Handler,
	(uint32_t *) SysTick_Handler,
	/*External interrupts*/
	(uint32_t *) WWDG_IRQHandler,                
    (uint32_t *) PVD_IRQHandler,                
    (uint32_t *) TAMP_STAMP_IRQHandler,          
    (uint32_t *) RTC_WKUP_IRQHandler,              
    (uint32_t *) FLASH_IRQHandler,                 
    (uint32_t *) RCC_IRQHandler,                   
    (uint32_t *) EXTI0_IRQHandler,                 
    (uint32_t *) EXTI1_IRQHandler,                 
    (uint32_t *) EXTI2_IRQHandler,                 
    (uint32_t *) EXTI3_IRQHandler,                
    (uint32_t *) EXTI4_IRQHandler,                 
    (uint32_t *) DMA1_Stream0_IRQHandler,          
    (uint32_t *) DMA1_Stream1_IRQHandler,          
    (uint32_t *) DMA1_Stream2_IRQHandler,          
    (uint32_t *) DMA1_Stream3_IRQHandler,          
    (uint32_t *) DMA1_Stream4_IRQHandler,          
    (uint32_t *) DMA1_Stream5_IRQHandler,          
    (uint32_t *) DMA1_Stream6_IRQHandler,          
    (uint32_t *) ADC_IRQHandler,                   
    (uint32_t *) CAN1_TX_IRQHandler,               
    (uint32_t *) CAN1_RX0_IRQHandler,              
    (uint32_t *) CAN1_RX1_IRQHandler,              
    (uint32_t *) CAN1_SCE_IRQHandler,              
    (uint32_t *) EXTI9_5_IRQHandler,               
    (uint32_t *) TIM1_BRK_TIM9_IRQHandler,         
    (uint32_t *) TIM1_UP_TIM10_IRQHandler,         
    (uint32_t *) TIM1_TRG_COM_TIM11_IRQHandler,    
    (uint32_t *) TIM1_CC_IRQHandler,               
    (uint32_t *) TIM2_IRQHandler,                  
    (uint32_t *) TIM3_IRQHandler,                  
    (uint32_t *) TIM4_IRQHandler,                  
    (uint32_t *) I2C1_EV_IRQHandler,               
    (uint32_t *) I2C1_ER_IRQHandler,               
    (uint32_t *) I2C2_EV_IRQHandler,               
    (uint32_t *) I2C2_ER_IRQHandler,               
    (uint32_t *) SPI1_IRQHandler,                  
    (uint32_t *) SPI2_IRQHandler,                  
    (uint32_t *) USART1_IRQHandler,                
    (uint32_t *) USART2_IRQHandler,                
    (uint32_t *) USART3_IRQHandler,                
    (uint32_t *) EXTI15_10_IRQHandler,             
    (uint32_t *) RTC_Alarm_IRQHandler,             
    (uint32_t *) OTG_FS_WKUP_IRQHandler,           
    (uint32_t *) TIM8_BRK_TIM12_IRQHandler,        
    (uint32_t *) TIM8_UP_TIM13_IRQHandler,         
    (uint32_t *) TIM8_TRG_COM_TIM14_IRQHandler,   
    (uint32_t *) TIM8_CC_IRQHandler,               
    (uint32_t *) DMA1_Stream7_IRQHandler,          
    (uint32_t *) FSMC_IRQHandler,                  
    (uint32_t *) SDIO_IRQHandler,                  
    (uint32_t *) TIM5_IRQHandler,                  
    (uint32_t *) SPI3_IRQHandler,                  
    (uint32_t *) UART4_IRQHandler,                 
    (uint32_t *) UART5_IRQHandler,                 
    (uint32_t *) TIM6_DAC_IRQHandler,              
    (uint32_t *) TIM7_IRQHandler,                  
    (uint32_t *) DMA2_Stream0_IRQHandler,          
    (uint32_t *) DMA2_Stream1_IRQHandler,          
    (uint32_t *) DMA2_Stream2_IRQHandler,          
    (uint32_t *) DMA2_Stream3_IRQHandler,          
    (uint32_t *) DMA2_Stream4_IRQHandler,          
    (uint32_t *) ETH_IRQHandler,                   
    (uint32_t *) ETH_WKUP_IRQHandler,              
    (uint32_t *) CAN2_TX_IRQHandler,               
    (uint32_t *) CAN2_RX0_IRQHandler,              
    (uint32_t *) CAN2_RX1_IRQHandler,              
    (uint32_t *) CAN2_SCE_IRQHandler,              
    (uint32_t *) OTG_FS_IRQHandler,                
    (uint32_t *) DMA2_Stream5_IRQHandler,          
    (uint32_t *) DMA2_Stream6_IRQHandler,          
    (uint32_t *) DMA2_Stream7_IRQHandler,          
    (uint32_t *) USART6_IRQHandler,                
    (uint32_t *) I2C3_EV_IRQHandler,               
    (uint32_t *) I2C3_ER_IRQHandler,               
    (uint32_t *) OTG_HS_EP1_OUT_IRQHandler,        
    (uint32_t *) OTG_HS_EP1_IN_IRQHandler,         
    (uint32_t *) OTG_HS_WKUP_IRQHandler,           
    (uint32_t *) OTG_HS_IRQHandler,                
    (uint32_t *) DCMI_IRQHandler,                  
    (uint32_t *) 0,                                
    (uint32_t *) HASH_RNG_IRQHandler,              
    (uint32_t *) FPU_IRQHandler,                   
};
