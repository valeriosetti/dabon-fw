/*****************************************
	Printf() functions for debug support
******************************************/

#ifndef DEBUG_PRINTF_H_
#define DEBUG_PRINTF_H_

#include <stdarg.h>

int debug_printf(const char *format, ...);
// int debug_sprintf(char *out, const char *format, ...);

#endif /* DEBUG_PRINTF_H_ */
