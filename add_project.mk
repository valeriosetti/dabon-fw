PROJECT_PATH=project

SRCS += $(PROJECT_PATH)/sources/clock_configuration.c
SRCS += $(PROJECT_PATH)/sources/debug_printf.c
SRCS += $(PROJECT_PATH)/sources/eeprom.c
SRCS += $(PROJECT_PATH)/sources/fsmc.c
SRCS += $(PROJECT_PATH)/sources/i2c.c
SRCS += $(PROJECT_PATH)/sources/main.c
SRCS += $(PROJECT_PATH)/sources/oled.c
SRCS += $(PROJECT_PATH)/sources/output_i2s.c
SRCS += $(PROJECT_PATH)/sources/sd_card.c
SRCS += $(PROJECT_PATH)/sources/sdio.c
SRCS += $(PROJECT_PATH)/sources/sd_card_detect.c
SRCS += $(PROJECT_PATH)/sources/spi.c
SRCS += $(PROJECT_PATH)/sources/timer.c
SRCS += $(PROJECT_PATH)/sources/tuner.c
SRCS += $(PROJECT_PATH)/sources/uart.c
SRCS += $(PROJECT_PATH)/sources/interrupts.c
SRCS += $(PROJECT_PATH)/sources/systick.c
SRCS += $(PROJECT_PATH)/sources/mp3_player.c
				
INCS += -I$(PROJECT_PATH)/includes
