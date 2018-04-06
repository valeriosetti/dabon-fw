#include "mp3_player.h"
#include "mp3dec.h"
#include "ff.h"
#include "debug_printf.h"
#include "string.h"
#include "kernel.h"
#include "output_i2s.h"
#include "utils.h"

#define debug_msg(format, ...)		debug_printf("[mp3_player] " format, ##__VA_ARGS__)

ALLOCATE_TASK(mp3_player, 5);

HMP3Decoder hMP3Decoder = NULL;
uint8_t internal_status = MP3_PLAYER_IDLE;
FIL fp;

#define FILE_BUFFER_SIZE		4096
struct {
	uint8_t data[FILE_BUFFER_SIZE];
	uint16_t write_ptr;		
	uint16_t read_ptr;
	uint32_t total_read_bytes;	
} local_buffer;
#define mp3_player_get_internal_buffer_data_count()			(local_buffer.write_ptr - local_buffer.read_ptr)

#define OUTPUT_AUDIO_SAMPLES_MAX_SIZE 		2048
uint16_t output_audio_samples[OUTPUT_AUDIO_SAMPLES_MAX_SIZE];

//>>> DEBUG
/*int16_t sine_look_up_table[] = {
		0x8000,0x90b5,0xa120,0xb0fb,0xbfff,0xcdeb,0xda82,0xe58c,
		0xeed9,0xf641,0xfba2,0xfee7,0xffff,0xfee7,0xfba2,0xf641,
		0xeed9,0xe58c,0xda82,0xcdeb,0xbfff,0xb0fb,0xa120,0x90b5,
		0x8000,0x6f4a,0x5edf,0x4f04,0x4000,0x3214,0x257d,0x1a73,
		0x1126,0x9be,0x45d,0x118,0x0,0x118,0x45d,0x9be,
		0x1126,0x1a73,0x257d,0x3214,0x4000,0x4f04,0x5edf,0x6f4a
};
int16_t tone_buffer[2*array_size(sine_look_up_table)];
static void initialize_buffers()
{
	uint16_t index;
	for (index=0; index<array_size(sine_look_up_table); index++) {
		tone_buffer[2*index] = tone_buffer[2*index+1] = (int16_t)(sine_look_up_table[index] - 0x8000);
	}
}*/
//<<< DEBUG

/*******************************************************************/
/*		INTERNAL FUNCTIONS
/*******************************************************************/
/*
 * Refill the internal buffer
 */
static int32_t mp3_player_refill_buffer()
{
	// If the buffer has already been used, the "read_ptr" will not be set at 
	// the beginning of the array. Therefore the data must be lef-shifted before
	// parsing more data from the file.
	if (local_buffer.read_ptr != 0) {
		uint16_t bytes_to_shift = local_buffer.write_ptr - local_buffer.read_ptr;
		memcpy(&local_buffer.data[0], &local_buffer.data[local_buffer.read_ptr], bytes_to_shift);
		local_buffer.read_ptr = 0;
		local_buffer.write_ptr = bytes_to_shift;
	}
	
	// Now fill the remaining part of the local buffer
	uint32_t read_bytes = 0;
	uint32_t bytes_to_read = FILE_BUFFER_SIZE - local_buffer.write_ptr;
	
	// If there's nothing to read then return immediately
	if (bytes_to_read == 0) {
		return 0;
	}
	
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
    local_buffer.write_ptr += read_bytes;
    local_buffer.total_read_bytes += read_bytes;
    
	return 0;	
}

/*
 * 	"left" shift the buffer and refill with new data
 */
static int32_t mp3_player_seek(uint16_t offset)
{
	// it's not possible to seek over 
	if (offset > FILE_BUFFER_SIZE)
		offset = FILE_BUFFER_SIZE;
		
	local_buffer.read_ptr = offset;
	
	// pointers' integrity check
	if (local_buffer.write_ptr < local_buffer.read_ptr)
		local_buffer.write_ptr = local_buffer.read_ptr;
		
	return mp3_player_refill_buffer();
}

/*
 * 	Reset the internal buffer infos
 */
static int32_t mp3_player_reset_internal_buffer()
{
	memset(local_buffer.data, 0, sizeof(local_buffer.data));
	local_buffer.read_ptr = 0;
	local_buffer.write_ptr = 0;
	local_buffer.total_read_bytes = 0;
	return 0;
}

/*******************************************************************/
/*		TASK RELATED FUNCTIONS
/*******************************************************************/
int32_t mp3_player_task_func()
{
	MP3FrameInfo frame_info;
	uint8_t* data_buff_ptr;
	
	// decode the current frame
	data_buff_ptr = &(local_buffer.data[local_buffer.read_ptr]);
	int available_samples = mp3_player_get_internal_buffer_data_count();
	int32_t ret_val = MP3Decode(hMP3Decoder, &data_buff_ptr, &available_samples, output_audio_samples, 0);
	if (ret_val < 0) {
		//mp3_player_stop();
		debug_msg("error: decoding frame (%d) - read_bytes=%d\n", ret_val, local_buffer.total_read_bytes);
		//return DIE;
	} 
	
	// enqueue decoded samples
	if (ret_val >= 0) {
		local_buffer.read_ptr += mp3_player_get_internal_buffer_data_count() - available_samples;
		MP3GetLastFrameInfo(hMP3Decoder, &frame_info);
		output_i2s_enqueue_samples(output_audio_samples, frame_info.outputSamps);
	} else {
		local_buffer.read_ptr += 10;
	}
	
	// get infos for the next frame
	data_buff_ptr = &(local_buffer.data[local_buffer.read_ptr]);
	int32_t sync_word_offset = MP3FindSyncWord(data_buff_ptr, mp3_player_get_internal_buffer_data_count());
	if (sync_word_offset < 0) {
		mp3_player_stop();
		debug_msg("error: unable to find new sync word\n");
		return DIE;
	}
	local_buffer.read_ptr += sync_word_offset;
	data_buff_ptr += sync_word_offset;
	MP3GetNextFrameInfo(hMP3Decoder, &frame_info, data_buff_ptr);
	
	if (local_buffer.read_ptr > FILE_BUFFER_SIZE/2) {
		if (mp3_player_refill_buffer() < 0) {
			mp3_player_stop();
			debug_msg("error: unable to refill the buffer\n");
			return DIE;
		}
	}
	
	// if the output audio buffer is still partially free then reschedule immediately,
	// otherwise wait for the callback
	if (output_i2s_get_buffer_free_space() > frame_info.outputSamps) {
		return IMMEDIATELY;
	} else {
		return WAIT_FOR_RESUME;
	}
}

/*
 * Callback function from the output_i2s
 */
void mp3_player_request_audio_samples()
{
	kernel_activate_task_immediately(&mp3_player_task);
}

/*******************************************************************/
/*		PUBLIC FUNCTIONS
/*******************************************************************/
/*
 * 
 */
int32_t mp3_player_play(char* path)
{
	if (hMP3Decoder == NULL)
		hMP3Decoder = MP3InitDecoder();
	
	// try to open the file
	if (f_open(&fp, path, FA_READ) != FR_OK) {
		debug_msg("error opening the file\n");
		return -1;
	}
	
	mp3_player_reset_internal_buffer();
	
	// fill the internal file buffer and ensure that the local buffer 
	// starts with a correct frame header
	int32_t sync_word_offset;
	do {
		if (mp3_player_refill_buffer() < 0) {
			return -2;
		}
		sync_word_offset = MP3FindSyncWord(local_buffer.data, local_buffer.write_ptr);
		// sync word not found! Clear the current buffer and repeat the procedure
		if (sync_word_offset < 0) {
			mp3_player_reset_internal_buffer();
		}
	} while (sync_word_offset < 0);
	
	// refill the buffer
	local_buffer.read_ptr += sync_word_offset;
	mp3_player_refill_buffer();
	
	// start the playback by activating the callback
	internal_status = MP3_PLAYER_PLAYING;
	output_i2s_register_callback(mp3_player_request_audio_samples);

	return 0;
}

/*
 * 
 */
int32_t mp3_player_pause()
{
	output_i2s_register_callback(NULL);
	internal_status = MP3_PLAYER_PAUSED;
	return 0;
}

/*
 * 
 */
int32_t mp3_player_resume()
{
	output_i2s_register_callback(mp3_player_request_audio_samples);
	internal_status = MP3_PLAYER_PLAYING;
	return 0;
}

/*
 *
 */
int32_t mp3_player_stop()
{
	output_i2s_register_callback(NULL);
	internal_status = MP3_PLAYER_IDLE;
	return 0;
}

/*
 * 
 */
uint8_t mp3_player_get_status()
{
	return internal_status;
}
