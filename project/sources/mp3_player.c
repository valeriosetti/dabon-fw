#include "mp3_player.h"
#include "mp3dec.h"
#include "ff.h"
#include "debug_printf.h"
#include "string.h"

#define debug_msg(...)		debug_printf_with_tag("[mp3_player] ", __VA_ARGS__)

HMP3Decoder hMP3Decoder = NULL;
uint32_t internal_status = MP3_PLAYER_IDLE;
FIL fp;

#define FILE_BUFFER_SIZE		2048
struct {
	uint8_t data[FILE_BUFFER_SIZE];
	uint16_t write_ptr;		
	uint16_t read_ptr;		
} local_buffer;

/*******************************************************************/
/*		INTERNAL FUNCTIONS
/*******************************************************************/
/*
 * Refill the internal buffer
 */
static uint32_t mp3_player_refill_buffer()
{
	// If the buffer has already been used, the "read_ptr" will not be set at 
	// the beginning of the array. Therefore the data must be lef-shifted before
	// parsing more data from the file.
	if ((local_buffer.read_ptr != 0) && (local_buffer.write_ptr > local_buffer.read_ptr)) {
		uint16_t bytes_to_shift = local_buffer.write_ptr - local_buffer.read_ptr;
		memcpy(&local_buffer.data[0], &local_buffer.data[local_buffer.read_ptr], bytes_to_shift);
		local_buffer.read_ptr = 0;
		local_buffer.write_ptr = bytes_to_shift;
	}
	
	// Now fill the remaining part of the local buffer
	uint32_t read_bytes = 0;
	uint32_t bytes_to_read = FILE_BUFFER_SIZE - local_buffer.write_ptr;
	if ( !f_eof(&fp) ) {
		if (f_read(&fp, &local_buffer.data[local_buffer.write_ptr], bytes_to_read, (UINT*)&read_bytes) != 0) {
			debug_msg("error reading the file\n");
			return -1;
		}
		if (read_bytes == 0) {
			debug_msg("0 bytes read even if no EOF was reached\n");
			return -2;
		}
	}
		
}

/*
 * 	Reset the internal buffer infos
 */
static uint32_t mp3_player_reset_internal_buffer()
{
	memset(local_buffer.data, 0, sizeof(local_buffer.data));
	local_buffer.read_ptr = 0;
	local_buffer.write_ptr = 0;
}

/*******************************************************************/
/*		PUBLIC FUNCTIONS
/*******************************************************************/
/*
 * 
 */
uint32_t mp3_player_play(char* path)
{
	if (hMP3Decoder == NULL)
		hMP3Decoder = MP3InitDecoder;
	
	// try to open the file
	if (f_open(&fp, path, FA_READ) != FR_OK) {
		debug_msg("error opening the file\n");
		return -1;
	}
	
	mp3_player_reset_internal_buffer();
	if (mp3_player_refill_buffer() != 0)
		return -2;
		
	if (MP3FindSyncWord(local_buffer.data, local_buffer.write_ptr) != ERR_MP3_NONE) {
		debug_msg("cannot find sync word\n");
		return -3;
	}
	
	internal_status = MP3_PLAYER_PLAYING;
}

/*
 * 
 */
uint32_t mp3_player_pause()
{
	internal_status = MP3_PLAYER_PAUSED;
}

/*
 * 
 */
uint32_t mp3_player_stop()
{
	internal_status = MP3_PLAYER_IDLE;
}

/*
 * 
 */
uint8_t mp3_player_get_status()
{
	return internal_status;
}
