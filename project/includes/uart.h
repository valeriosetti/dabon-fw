#ifndef _UART_H_
#define _UART_H_

#include "stdint.h"

// Functions' return codes
#define UART_SUCCECSS	0L
#define UART_ERROR		-1L

int uart_init(void);
int uart_put_char(uint8_t c);

#endif /* _UART_H_ */
