FATFS_PATH = ./FatFs_0.13/source

INCS += -I$(FATFS_PATH)

SRCS += $(wildcard $(FATFS_PATH)/*.c) 	
