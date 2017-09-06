#ifndef _MP3_PLAYER_H_
#define _MP3_PLAYER_H_

#include "stdint.h"

uint32_t mp3_player_play(char* path);
uint32_t mp3_player_pause(void);
uint32_t mp3_player_stop(void);
uint8_t mp3_player_get_status(void);

#define MP3_PLAYER_IDLE		0x00
#define MP3_PLAYER_PLAYING	0x01
#define MP3_PLAYER_PAUSED	0x02
#define MP3_PLAYER_ERROR	0x03

#endif //_MP3_PLAYER_H_
