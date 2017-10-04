#ifndef _UTILS_H_
#define _UTILS_H_

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
#define NULL		((void*)0)
#endif

#define TRUE		1UL
#define FALSE		0UL

#define array_size(_x_)		(sizeof(_x_)/sizeof(_x_[0]))

int reset(int argc, char *argv[]);

#endif // _UTILS_H_
