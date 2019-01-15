#ifndef _UTILS_H_
#define _UTILS_H_

#include "stdint.h"

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define CLEAR_REG(REG)        ((REG) = (0x0))
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#define READ_REG(REG)         ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))
#define UNUSED(x) ((void)(x))

#ifndef NULL
#define NULL	(void*)0
#endif

#define FALSE	(0UL)
#define TRUE 	(!FALSE)

#define array_size(_x_)		(sizeof(_x_)/sizeof(_x_[0]))

int reset(int argc, char *argv[]);

typedef enum {
	SI468X_BOOT_FW,
	SI468X_FM_FW,
	SI468X_DAB_FW
} Tuner_FW_type;
	
int8_t utils_is_fw_embedded(Tuner_FW_type type);
uint8_t* utils_get_embedded_FW_start_address(Tuner_FW_type type);
uint32_t utils_get_embedded_FW_size(Tuner_FW_type type);

#endif // _UTILS_H_
