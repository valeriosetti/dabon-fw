# The following option selects which firmware will be loaded into the
# tuner. Allowed options are "FM_RADIO", "DAB_RADIO" and "NO_EXT_FIRMWARES"
TUNER_CONFIG=NO_EXT_FIRMWARES

# Include project's sources and includes
include ./add_project.mk
include ./add_ST.mk
include ./add_external_firmwares.mk
include ./add_FatFs.mk
#include ./add_helix.mk
include ./add_libmad.mk

# Binaries will be generated with this name (.elf, .bin, .hex, etc)
PROJ_NAME = dabon
DEVICE_TYPE = STM32F407xx
OUT_PATH = ./build
OBJ_PATH = ./build/objs

# Convert the list of source files to a list of object files
C_OBJS = $(addprefix $(OBJ_PATH)/,$(SRCS:.c=.o))
ASM_OBJS = $(addprefix $(OBJ_PATH)/, $(ASMS:.s=.o))
FWS_OBJS = $(addprefix $(OBJ_PATH)/, $(FWS:.bin=.o))
CONV_IMGS = $(IMAGES:.bmp=.h)

# Add to VPATH the list of directories for source files
VPATH = $(dir $(SRCS)) \
		$(dir $(ASMS)) \
		$(dir $(FWS))

#######################################################################################
TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi-7-2018-q2-update
LIBC_PATH=$(TOOLCHAIN_PATH)/arm-none-eabi/lib/thumb/v7e-m/fpv4-sp/hard
CC=$(TOOLCHAIN_PATH)/bin/arm-none-eabi-gcc
AS=$(TOOLCHAIN_PATH)/bin/arm-none-eabi-as
SIZE=$(TOOLCHAIN_PATH)/bin/arm-none-eabi-size
OBJCOPY=$(TOOLCHAIN_PATH)/bin/arm-none-eabi-objcopy
LD=$(TOOLCHAIN_PATH)/bin/arm-none-eabi-ld

# compiler options
C_FLAGS  = -g  
C_FLAGS += -O0
C_FLAGS += -Werror
C_FLAGS += -mlittle-endian
C_FLAGS += -mthumb
C_FLAGS += -mcpu=cortex-m4
C_FLAGS += -march=armv7e-m
C_FLAGS += -mfloat-abi=hard
C_FLAGS += -mfpu=fpv4-sp-d16
C_FLAGS += -D$(DEVICE_TYPE)
C_FLAGS += -DFPM_DEFAULT -DNDEBUG -DHAVE_CONFIG_H
C_FLAGS += -MD -MP -MF .dep/$(@F).d
C_FLAGS += -D$(TUNER_CONFIG)
C_FLAGS += -ffreestanding
#C_FLAGS += -Wall

LINKER_FLAGS = -Wl,-Map=$(OUT_PATH)/$(PROJ_NAME).map,--cref,--print-memory-usage
LINKER_FLAGS += -nostartfiles

###############################################################################
.PHONY: check_flags clean_images check_output_folders

all : check_flags check_output_folders $(CONV_IMGS) $(OUT_PATH)/$(PROJ_NAME).elf
	@echo "Creating HEX and BIN files"
	@$(OBJCOPY) -O ihex $(OUT_PATH)/$(PROJ_NAME).elf $(OUT_PATH)/$(PROJ_NAME).hex
	@$(OBJCOPY) -O binary $(OUT_PATH)/$(PROJ_NAME).elf $(OUT_PATH)/$(PROJ_NAME).bin
#	@$(SIZE) -A -x $(OUT_PATH)/$(PROJ_NAME).elf

check_flags:
ifneq ($(TUNER_CONFIG),DAB_RADIO)
ifneq ($(TUNER_CONFIG),FM_RADIO)
ifneq ($(TUNER_CONFIG),NO_EXT_FIRMWARES)
	$(error Specified tuner firmware was $(TUNER_CONFIG), but it can be either FM_RADIO or DAB_RADIO)
endif
endif
endif
	@echo "Tuner version --> $(TUNER_CONFIG)"
	
check_output_folders:
	if [ ! -d "./build" ]; then mkdir "build"; fi
	if [ ! -d "./build/objs" ]; then mkdir "build/objs"; fi
	
$(CONV_IMGS) : %.h : %.bmp
	@python3 $(IMAGE_CONVERTER_SCRIPT)  $< $@

$(OUT_PATH)/$(PROJ_NAME).elf : $(C_OBJS) $(ASM_OBJS) $(FWS_OBJS)
#	@echo $(CFLAGS)
	@echo "Assembling objects"
	@$(CC) -T$(LINKER) -L $(LIBC_PATH) $(LINKER_FLAGS) $^ -o $@ 
	
$(FWS_OBJS) : $(OBJ_PATH)/%.o : %.bin
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@echo "Processing"   $<
	@$(OBJCOPY)	-I binary -O elf32-littlearm -B arm --rename-section .data=.text $< $@

$(C_OBJS) : $(OBJ_PATH)/%.o : %.c
#	@echo $(INCS)
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@echo "Compiling"   $<
	@$(CC) -c $(C_FLAGS) $(INCS) $< -o $@
	
$(ASM_OBJS) : $(OBJ_PATH)/%.o : %.s
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@echo "Compiling " $<
	@$(CC) -c $(C_FLAGS) $(INCS) $< -o $@

clean_images:
	rm -f $(CONV_IMGS)

clean: clean_images
	rm -rf $(OBJ_PATH)/*
	rm -f $(OUT_PATH)/$(PROJ_NAME).*
	rm -f .dep/*

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
