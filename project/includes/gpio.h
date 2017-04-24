#ifndef _GPIO_H_
#define _GPIO_H_

// MODER configuration bits
#define MODER_INPUT						0x00
#define MODER_GENERAL_PURPOSE_OUTPUT	0x01
#define MODER_ALTERNATE					0x02
#define MODER_ANALOG					0x03

// OSPEEDR configuration bits
#define OSPEEDR_2MHZ			0x00
#define OSPEEDR_25MHZ			0x01
#define OSPEEDR_50MHZ			0x02
#define OSPEED_100MHZ			0x03

// PUPDR configuration bits
#define PUPDR_NO_PULL		0x00
#define PUPDR_PULL_UP		0x01
#define PUPDR_PULL_DOWN		0x02

#endif // _GPIO_H_
