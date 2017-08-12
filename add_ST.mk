STM_CUBE_PATH = ./STM32Cube_FW_F4_V1.14.0

INCS += -I$(STM_CUBE_PATH)/Drivers/CMSIS/Device		\
		-I$(STM_CUBE_PATH)/Drivers/CMSIS/Include

#ASMS = $(STM_CUBE_PATH)/Drivers/CMSIS/Device/startup_stm32f407xx.s

LINKER = $(STM_CUBE_PATH)/STM32F407VGTx_FLASH.ld
