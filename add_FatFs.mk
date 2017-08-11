FATFS_PATH = ./FatFs_0.13/source

INCS += -I$(FATFS_PATH)

SRCS += $(FATFS_PATH)/diskio.c
SRCS += $(FATFS_PATH)/ff.c
SRCS += $(FATFS_PATH)/ffsystem.c
SRCS += $(FATFS_PATH)/ffunicode.c
