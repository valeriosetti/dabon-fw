#include "mp3_player.h"
#include "decoder.h"
#include "ff.h"
#include "debug_printf.h"
#include "string.h"
#include "kernel.h"
#include "output_i2s.h"
#include "utils.h"

#define debug_msg(format, ...)		debug_printf("[mp3_player] " format, ##__VA_ARGS__)

ALLOCATE_TASK(mp3_player, 5);

uint8_t internal_status = MP3_PLAYER_IDLE;
FIL fp;

#define FILE_BUFFER_SIZE		4096
uint8_t file_buffer[FILE_BUFFER_SIZE];

#define OUTPUT_AUDIO_SAMPLES_MAX_SIZE 		(1152)
audio_sample_t output_audio_samples[OUTPUT_AUDIO_SAMPLES_MAX_SIZE];

struct mad_stream mad_stream;
struct mad_frame mad_frame;
struct mad_synth mad_synth;

#define clip_audio_sample(sample) \
	do { \
		if (sample >= MAD_F_ONE) \
			sample = MAD_F_ONE - 1; \
		else if (sample < -MAD_F_ONE) \
			sample = -MAD_F_ONE; \
	} while (0) 

//>>> DEBUG
/*int16_t sine_look_up_table[] = {
		0x8000,0x90b5,0xa120,0xb0fb,0xbfff,0xcdeb,0xda82,0xe58c,
		0xeed9,0xf641,0xfba2,0xfee7,0xffff,0xfee7,0xfba2,0xf641,
		0xeed9,0xe58c,0xda82,0xcdeb,0xbfff,0xb0fb,0xa120,0x90b5,
		0x8000,0x6f4a,0x5edf,0x4f04,0x4000,0x3214,0x257d,0x1a73,
		0x1126,0x9be,0x45d,0x118,0x0,0x118,0x45d,0x9be,
		0x1126,0x1a73,0x257d,0x3214,0x4000,0x4f04,0x5edf,0x6f4a
};
audio_sample_t tone_buffer[array_size(sine_look_up_table)];
static void initialize_buffers()
{
	uint16_t index;
	for (index=0; index<array_size(sine_look_up_table); index++) {
		tone_buffer[index].left_ch = tone_buffer[index].right_ch = (int16_t)(sine_look_up_table[index] - 0x8000);
	}
}*/
//<<< DEBUG

/*******************************************************************/
/*		INTERNAL FUNCTIONS
/*******************************************************************/
/*
 * Refill the internal buffer
 */
static int32_t mp3_player_refill_buffer(uint8_t adjust_offsets)
{
	uint32_t bytes_to_drop = 0;
	uint32_t bytes_to_keep = 0;
	uint32_t read_bytes;
	
	// return a failure if the buffer cannot be filled with new data
	if (f_eof(&fp)) {
		debug_msg("EOF reached - cannot add more data to the buffer\n");
		return -1;
	}
	
	if (adjust_offsets) {
		bytes_to_drop = mad_stream.this_frame - mad_stream.buffer;
		bytes_to_keep = mad_stream.bufend - mad_stream.this_frame;
		memcpy((void*)mad_stream.buffer, mad_stream.this_frame, bytes_to_keep);
		f_read(&fp, file_buffer + bytes_to_keep, bytes_to_drop, (unsigned int*)&read_bytes);
	} else {
		f_read(&fp, file_buffer + bytes_to_keep, FILE_BUFFER_SIZE, (unsigned int*)&read_bytes);
	}
	
	//debug_msg("Read bytes = %u (%u%%)\n", read_bytes, (f_tell(&fp)*100)/(f_size(&fp)));
	
	if (adjust_offsets) {
		mad_stream.this_frame -= bytes_to_drop;
		mad_stream.next_frame -= bytes_to_drop;
		mad_stream.ptr.byte -= bytes_to_drop;
	}
	
	return 0;
}

static int32_t mp3_player_enqueue_decoded_audio_samples()
{
	// for now only stereo files are supported
	if (mad_synth.pcm.channels < 2)
		return -1;
		
	mad_fixed_t* pcm0_ptr = mad_synth.pcm.samples[0];
	mad_fixed_t* pcm1_ptr = mad_synth.pcm.samples[1];
	audio_sample_t* output_buf_ptr = output_audio_samples;
		
	// todo: downscale the decoded audio samples before streaming them!!!
	register uint16_t curr_sample;
	for (curr_sample = 0; curr_sample < mad_synth.pcm.length; curr_sample++, output_buf_ptr++) {
		clip_audio_sample(*pcm0_ptr);
		output_buf_ptr->left_ch = (int16_t)((*pcm0_ptr) >> (MAD_F_FRACBITS + 1 - 16));
		pcm0_ptr++;
		
		clip_audio_sample(*pcm1_ptr);
		output_buf_ptr->right_ch = (int16_t)((*pcm1_ptr) >> (MAD_F_FRACBITS + 1 - 16));
		pcm1_ptr++;
	}
	
	output_i2s_enqueue_samples(output_audio_samples, mad_synth.pcm.length);	
}

/*******************************************************************/
/*		TASK RELATED FUNCTIONS
/*******************************************************************/
int32_t mp3_player_task_func()
{
	uint8_t* data_buff_ptr;
	
	//decode the current frame
	if (mad_frame_decode(&mad_frame, &mad_stream) == -1) {
		if (!MAD_RECOVERABLE(mad_stream.error)) {
			debug_msg("Major error (%x): %s\n", mad_stream.error, mad_stream_errorstr(&mad_stream));
			mp3_player_stop();
			return DIE;
		} else {
			//debug_msg("Minor error (%x): %s\n", mad_stream.error, mad_stream_errorstr(&mad_stream));
			// refill the file buffer
			mp3_player_refill_buffer(TRUE);
			return IMMEDIATELY;
		}
	}
	
	// enqueue decoded audio samples
	mad_synth_frame(&mad_synth, &mad_frame);
	mp3_player_enqueue_decoded_audio_samples();	
	
	// refill the file buffer
	mp3_player_refill_buffer(TRUE);
	
	// if the output audio buffer is still partially free then reschedule immediately,
	// otherwise wait for the callback
	if (output_i2s_get_buffer_free_space() > 1152) {
		return IMMEDIATELY;
	} else {
		return WAIT_FOR_RESUME;
	}
	
	/*if (output_i2s_get_buffer_free_space() > array_size(tone_buffer)) {
		output_i2s_enqueue_samples(tone_buffer, array_size(tone_buffer));
		return IMMEDIATELY;
	} else {
		return WAIT_FOR_RESUME;
	}*/
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
	//initialize_buffers();
	
	// Initialize MAD library
    mad_stream_init(&mad_stream);
    mad_synth_init(&mad_synth);
    mad_frame_init(&mad_frame);
    
	// try to open the file
	if (f_open(&fp, path, FA_READ) != FR_OK) {
		debug_msg("error opening the file\n");
		return -1;
	}
	
	if (mp3_player_refill_buffer(FALSE) < 0) {
		debug_msg("unable to fill the internal buffer\n");
		return -1;
	}
	mad_stream_buffer(&mad_stream, file_buffer, FILE_BUFFER_SIZE);
	mad_stream_sync(&mad_stream);
	
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
