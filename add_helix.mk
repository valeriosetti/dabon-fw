HELIX_PATH=./helix_mp3

SRCS += $(HELIX_PATH)/mp3tabs.c
SRCS += $(HELIX_PATH)/mp3dec.c
SRCS += $(HELIX_PATH)/real/hufftabs.c
SRCS += $(HELIX_PATH)/real/stproc.c
SRCS += $(HELIX_PATH)/real/trigtabs_fixpt.c
SRCS += $(HELIX_PATH)/real/imdct.c
SRCS += $(HELIX_PATH)/real/huffman.c
SRCS += $(HELIX_PATH)/real/dct32.c
SRCS += $(HELIX_PATH)/real/bitstream.c
SRCS += $(HELIX_PATH)/real/scalfact.c
SRCS += $(HELIX_PATH)/real/dequant.c
SRCS += $(HELIX_PATH)/real/polyphase.c
SRCS += $(HELIX_PATH)/real/buffers.c
SRCS += $(HELIX_PATH)/real/subband.c
SRCS += $(HELIX_PATH)/real/dqchan.c
				
INCS += -I$(HELIX_PATH)/pub
INCS += -I$(HELIX_PATH)/real
