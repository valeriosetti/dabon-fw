# The following option selects which firmware will be loaded into the
# tuner. Allowed options are "FM_RADIO", "DAB_RADIO" and "NO_EXT_FIRMWARES"
TUNER_CONFIG=NO_EXT_FIRMWARES

# Include project's sources and includes
include ./add_project.mk
include ./add_ST.mk
include ./add_external_firmwares.mk
include ./add_FatFs.mk
include ./add_helix.mk

# Binaries will be generated with this name (.elf, .bin, .hex, etc)
PROJ_NAME = dabon
DEVICE_TYPE = STM32F407xx
OUT_PATH = ./build
OBJ_PATH = ./build/objs

# Convert the list of source files to a list of object files
C_OBJS = $(addprefix $(OBJ_PATH)/, $(notdir $(SRCS:.c=.o)))
ASM_OBJS = $(addprefix $(OBJ_PATH)/, $(notdir $(ASMS:.s=.o)))
FWS_OBJS = $(addprefix $(OBJ_PATH)/, $(notdir $(FWS:.bin=.o)))
CONV_IMGS = $(IMAGES:.bmp=.h)

# Add to VPATH the list of directories for source files
VPATH = $(dir $(SRCS)) \
		$(dir $(ASMS)) \
		$(dir $(FWS))

#######################################################################################
CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
SIZE=arm-none-eabi-size
OBJCOPY=arm-none-eabi-objcopy
LD=arm-none-eabi-ld

# compiler options
CFLAGS  = -g -O0 -T$(LINKER) -nostartfiles -Wl,-Map=$(OUT_PATH)/$(PROJ_NAME).map,--cref,--print-memory-usage 
CFLAGS += -Werror
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -D$(DEVICE_TYPE)
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += $(INCS)
CFLAGS += -MD -MP -MF .dep/$(@F).d
CFLAGS += -D$(TUNER_CONFIG)
# Removed options = -Wall  -mthumb-interwork --specs=nosys.specs 

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
	@$(CC) $(CFLAGS) $^ -o $@ 
	
$(FWS_OBJS) : $(OBJ_PATH)/%.o : %.bin
	@echo "Processing"   $<
	@$(OBJCOPY)	-I binary -O elf32-littlearm -B arm --rename-section .data=.text $< $@

$(C_OBJS) : $(OBJ_PATH)/%.o : %.c
#	@echo $(INCS)
	@echo "Compiling"   $<
	@$(CC) -c $(CFLAGS) $< -o $@
	
$(ASM_OBJS) : $(OBJ_PATH)/%.o : %.s
	@echo "Compiling " $<
	@$(CC) -c $(CFLAGS) $< -o $@

clean_images:
	rm -f $(CONV_IMGS)

clean: clean_images
	rm -f $(OBJ_PATH)/*.o
	rm -f $(OUT_PATH)/$(PROJ_NAME).*
	rm -f .dep/*

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
