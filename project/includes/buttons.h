#ifndef _BUTTONS_H_
#define _BUTTONS_H_

void buttons_init(void);
int buttons_scan(int argc, char *argv[]);

void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

#endif //_BUTTONS_H_
