#include <inttypes.h>
#include <unistd.h>
#include <FLAC/stream_decoder.h>
#include "mp3.h"
#include "wav.h"
//#include "alsa.h"
#include "constants.h"

// TODO 
int write_to_alsa( void *alsa_data, uint32_t *buffer, unsigned long length);

/**
 * FLAC write callback
 **/
FLAC__StreamDecoderWriteStatus write_callback( const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	size_t i;
	split_ch_t out_buffer[ frame->header.blocksize];
	struct information_buffer *info_buffer = client_data;
	( void)decoder;

	if( info_buffer->length == 0) 
	{
		fprintf( stderr, "FLAC frontend ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if( info_buffer->channels != 2) 
	{
		fprintf( stderr, "FLAC frontend ERROR: this example only supports stereo streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	/* write decoded PCM samples */
	info_buffer->time += frame->header.blocksize;
	fprintf( stdout, "\b\b\b\b\b%2i:%2i", ( info_buffer->time / 2646000), ((info_buffer->time) % 2646000) / 44100);
	for( i = 0; i < frame->header.blocksize; i++) 
	{
		out_buffer[ i].ch[ LEFT]  =  buffer[ LEFT ][ i];  
		out_buffer[ i].ch[ RIGHT] =  buffer[ RIGHT][ i];  
//		out_buffer[ i].ch[ LEFT]  =  buffer[ LEFT ][ i] * 0.9;  
//		out_buffer[ i].ch[ RIGHT] =  buffer[ RIGHT][ i] * 0.9;  
	}

    //fprintf (stderr, "FLAC: try wrote %d bytes\n", ( frame->header.blocksize) * 4);
    write_to_alsa( info_buffer->alsa_data, ( uint32_t *)out_buffer, frame->header.blocksize);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/**
 * FLAC error callback
 **/
void error_callback( const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	( void)decoder, ( void)client_data;

	fprintf( stderr, "FLAC frontend ERROR:Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[ status]);
}

/**
 * FLAC metadata callback
 **/
void metadata_callback( const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	( void)decoder; 
	struct information_buffer *info_buffer = client_data;

	/* print some stats */
	if ( metadata->type == FLAC__METADATA_TYPE_STREAMINFO) 
	{
		/* save for later */
		info_buffer->length = metadata->data.stream_info.total_samples;
		info_buffer->channels = metadata->data.stream_info.channels;

		fprintf( stdout, "sample rate    : %u Hz\n", metadata->data.stream_info.sample_rate);
		fprintf( stdout, "channels       : %u\n", info_buffer->channels);
		fprintf( stdout, "bits per sample: %u\n", metadata->data.stream_info.bits_per_sample);
		fprintf( stdout, "total time     : %lli:%lli\n", ( info_buffer->length / 2646000), (( info_buffer->length) % 2646000) / 44100);
	}
}

/**
 * main function for FLAC playback
 **/
int play_flac( char *filename, void *alsa_data)
{
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	struct information_buffer info_buffer;

	info_buffer.alsa_data = alsa_data;
	info_buffer.time = 0;
	info_buffer.channels = 0;

	if (( decoder = FLAC__stream_decoder_new()) == NULL) 
	{
		fprintf( stderr, "FLAC frontend ERROR: allocating FLAC decoder\n");
		return -1;
	}
	
	( void)FLAC__stream_decoder_set_md5_checking( decoder, true);

	init_status = FLAC__stream_decoder_init_file( decoder, filename,
												  write_callback, metadata_callback, 
												  error_callback, &info_buffer);
	if ( init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) 
	{
		fprintf( stderr, "FLAC frontend ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[ init_status]);
		ok = false;
	}

	if ( ok) 
	{
		ok = FLAC__stream_decoder_process_until_end_of_stream( decoder);
		fprintf( stdout, "\ndecoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf( stdout, "   state: %s\n", FLAC__StreamDecoderStateString[ FLAC__stream_decoder_get_state( decoder)]);
	}

	FLAC__stream_decoder_delete( decoder);
	return 1;
}
