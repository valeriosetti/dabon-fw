LIBMAD_PATH=./libmad-0.15.1b

SRCS += ${LIBMAD_PATH}/bit.c
SRCS += ${LIBMAD_PATH}/decoder.c
SRCS += ${LIBMAD_PATH}/fixed.c
SRCS += ${LIBMAD_PATH}/frame.c
SRCS += ${LIBMAD_PATH}/huffman.c
SRCS += ${LIBMAD_PATH}/layer3.c
SRCS += ${LIBMAD_PATH}/layer12.c
SRCS += ${LIBMAD_PATH}/stream.c
SRCS += ${LIBMAD_PATH}/synth.c
SRCS += ${LIBMAD_PATH}/timer.c
SRCS += ${LIBMAD_PATH}/version.c

INCS += -I$(LIBMAD_PATH)/
