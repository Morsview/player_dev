/**
 * mp3 module header 
 **/
#ifndef __MP3_H__
#define __MP3_H__

#include <mad.h>
#include <inttypes.h>

/**
 * This is a private message structure to carry 
 * information between callbacks. 
 **/
typedef struct information_buffer {
	unsigned char const *file;
	unsigned long long length;
	void    *alsa_data;
	uint32_t time;
	unsigned channels;
} information_buffer_t;

enum mad_flow mp3_decoder_input( void *data, struct mad_stream *stream);
enum mad_flow mp3_decoder_output( void *data, struct mad_header const *header, struct mad_pcm *pcm);
enum mad_flow mp3_decoder_error_handler( void *data, struct mad_stream *stream, struct mad_frame *frame);
int play_mp3( void *alsa_data, int file, unsigned long length);

#endif /* __MP3_H__ */
