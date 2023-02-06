#include <sys/mman.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <id3tag.h>
#include <unistd.h>
#include <libintl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <locale.h>
#include "mp3.h"
#include "wav.h"
#include "constants.h"

#define  _(text)	gettext(text)
#define N_(text)	(text)
static int const on_same_line = 0;

static enum mad_flow callback_header( void *data,
		struct mad_header const *header);
static void process_id3( struct id3_tag const *tag);
static void detail( char const *label, char const *format, ...);
static void error( char const *id, char const *format, ...);

// TODO 
int write_to_alsa( void *alsa_data, uint32_t *buffer, unsigned long length);

/**
 * Input MP3 callback.
 **/
enum mad_flow mp3_decoder_input( void *data, struct mad_stream *stream)
{
	struct information_buffer *info_buffer = data;
	const unsigned char *p;
	int diff = 0;

	if ( info_buffer->length > BUFF_SIZE)
	{
		diff = labs( stream->buffer - stream->next_frame);
		p = info_buffer->file + diff;
		mad_stream_buffer( stream, p, BUFF_SIZE );
	} else
	{	
		if (info_buffer->length <= BUFF_SIZE/4) 
		{
			return MAD_FLOW_STOP;
		} else
		{
			diff = info_buffer->length;
			p = info_buffer->file + diff; 
			mad_stream_buffer( stream, p, info_buffer->length);
		}
	}
	info_buffer->length = info_buffer->length - diff;
	info_buffer->file = p;

	return MAD_FLOW_CONTINUE;
}

/**
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples(32 bits each) down to 16 bits.
 **/
int16_t scale(mad_fixed_t sample)
{
	/* gain */
//	sample *= 6;
	
	/* round */
	if ( ( sample & 0xFFFF) > 0x7FFF)
	{
		sample >>= 16;
		sample ++;
	} else
	{ 
		sample >>= 16;
	}
	
	/* clip */
	if (sample >= INT16_MAX)
		sample = INT16_MAX;
	else if (sample < INT16_MIN)
		sample = INT16_MIN;

	return sample;
}

/**
 * Output MP3 callback function. 
 **/
enum mad_flow mp3_decoder_output( void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
	unsigned int nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	struct information_buffer *info_buffer = data;

	/* pcm->samplerate contains the sampling frequency */

	nsamples  = pcm->length;
	left_ch   = pcm->samples[ LEFT];
	right_ch  = pcm->samples[ RIGHT];
	split_ch_t out_buffer[ nsamples];

	for ( int i = 0; i < nsamples; i++)
	{
		out_buffer[ i].ch[ LEFT] = scale( *left_ch++) * 5;
		out_buffer[ i].ch[ RIGHT] = scale( *right_ch++) * 5;
	}

	info_buffer->time += nsamples;
	fprintf( stderr, "\b\b\b\b\b%2i:%2i", ( info_buffer->time / 2646000), ((info_buffer->time) % 2646000) / 44100);

	if ( write_to_alsa( info_buffer->alsa_data, ( uint32_t*)out_buffer, nsamples) == -1)
	{
		fprintf( stderr, "MP3: write to alsa failed.\n");
		//return MAD_FLOW_STOP;
	}

	return MAD_FLOW_CONTINUE;
}

/**
 * Error MP3 callback function.
 **/
enum mad_flow mp3_decoder_error_handler( void *data, struct mad_stream *stream, 
		struct mad_frame *frame)
{
	struct information_buffer *info_buffer = data;

	if ( ( stream->this_frame - info_buffer->file - info_buffer->length < 10) && 
			( stream->error == 0x0101 || stream->error == MAD_ERROR_BADBITRATE)) 
	{
		printf("\nExit MAD: end of file\n");
		return MAD_FLOW_STOP;
	}

//	fprintf( stderr, "decoding error 0x%04x (%s) at byte offset %ld, length %lld\n",
//		stream->error, mad_stream_errorstr(stream),
//		stream->this_frame - info_buffer->file,
//		info_buffer->length);


	switch( stream->error) {

		case MAD_ERROR_LOSTSYNC:
		case MAD_ERROR_BADCRC:
		case MAD_ERROR_BADBITRATE:
		case MAD_ERROR_BADDATAPTR:
			return MAD_FLOW_CONTINUE;

		default:
			if ( MAD_RECOVERABLE( stream->error)) {
				fprintf( stderr, "decoding error 0x%04x (%s) at byte offset %ld, length %lld\n",
						stream->error, mad_stream_errorstr(stream),
						stream->this_frame - info_buffer->file,
						info_buffer->length);
			} else {
				fprintf( stderr, "decoding error 0x%04x (%s) at byte offset %ld, length %lld\n",
						stream->error, mad_stream_errorstr(stream),
						stream->this_frame - info_buffer->file,
						info_buffer->length);
			}
			break;	
	}


	// Abort decoding if error is non-recoverable
	if ( MAD_RECOVERABLE ( stream->error))
		return MAD_FLOW_CONTINUE;
	else return MAD_FLOW_BREAK;
}

/**
 * Main function for the mp3 playback 
 **/
int play_mp3( void *alsa_data, int file, unsigned long length)
{
	void *fdm, *stream_ptr;
	int fd;
	struct id3_file *id3_file;
	
	setlocale(LC_ALL, "");

	fd = dup( file);
	id3_file = id3_file_fdopen( fd, ID3_FILE_MODE_READONLY);
	if ( id3_file == 0) {
		close( fd);
	}
	else {
		process_id3( id3_file_tag( id3_file));
	
		id3_file_close( id3_file);
	}

	fdm = mmap( 0, length, PROT_READ, MAP_SHARED, file, 0);
	if ( fdm == MAP_FAILED) return -3;

	stream_ptr = fdm;
	signed long header_len = id3_tag_query( stream_ptr, ID3_TAG_QUERYSIZE);
	if ( header_len > 0) 
	{
	  	//printf( "TAG QUERY RESULT is %ld\n", header_len);
	  	stream_ptr = ( void *)(( char *)stream_ptr + header_len); 
	} else 
	{
		header_len = 0;
	}

	struct mad_decoder decoder;	
	struct information_buffer info_buffer;
	int result;

	info_buffer.file  = stream_ptr;
	info_buffer.length = length - header_len - 64;
	info_buffer.alsa_data = alsa_data;
	info_buffer.time = 0;

	/* configure input, output, and error functions */
	mad_decoder_init( &decoder, &info_buffer,
		mp3_decoder_input, callback_header/* header */, 0 /* filter */, mp3_decoder_output,
		mp3_decoder_error_handler, 0 /* message */);

	/* start decoding */
	result = mad_decoder_run( &decoder, MAD_DECODER_MODE_SYNC);

	/* release the decoder */
	mad_decoder_finish(&decoder);

	return result;
}

static enum mad_flow callback_header( void *data,
		struct mad_header const *header)
{
	// Abort thread ?
//	if (terminate_decoder_thread)
//		return MAD_FLOW_STOP;

	//printf( "samplerate of file: %d\n", header->samplerate);
	if ( 44100 != header->samplerate) 
	{
		printf( "Sample rate of input file (%d) is different to 44100\n", 
						header->samplerate);
		
		return MAD_FLOW_BREAK;
	} 
	
	// Calculate framesize ?
//	if (input->framesize==0) {
//		int padding = 0;
//		if (header->flags&MAD_FLAG_PADDING) padding=1;
//		input->framesize = 144 * header->bitrate / (header->samplerate + padding);
//		if (verbose) printf( "Framesize: %d bytes.\n", input->framesize );
//	}
	
	
	// Calculate duration?
//	if (input->duration==0) {
//		int frames = (input->end_pos - input->start_pos) / input->framesize;
//		input->duration = (1152.0f * frames) / header->samplerate;
//		if (verbose) printf( "Duration: %2.2f seconds.\n", input->duration );
//	}
	
	// Header looks OK
	return MAD_FLOW_CONTINUE;
}

/*
 * NAME:	process_id3()
 * DESCRIPTION:	display and process ID3 tag information
 */
static void process_id3( struct id3_tag const *tag)
{
	unsigned int i;
	struct id3_frame const *frame;
	id3_ucs4_t const *ucs4;
	id3_latin1_t *latin1;
	//id3_utf8_t utf8_string[256];

	static struct {
	char const *id;
	char const *label;
	} const info[] = {
	{ ID3_FRAME_TITLE,  N_("Title")	 },
	{ "TIT3",		   0			   },  /* Subtitle */
	{ "TCOP",		   0			   },  /* Copyright */
	{ "TPRO",		   0			   },  /* Produced */
	{ "TCOM",		   N_("Composer")  },
	{ ID3_FRAME_ARTIST, N_("Artist")	},
	{ "TPE2",		   N_("Orchestra") },
	{ "TPE3",		   N_("Conductor") },
	{ "TEXT",		   N_("Lyricist")  },
	{ ID3_FRAME_ALBUM,  N_("Album")	 },
	{ ID3_FRAME_TRACK,  N_("Track")	 },
	{ ID3_FRAME_YEAR,   N_("Year")	  },
	{ "TPUB",		   N_("Publisher") },
	{ ID3_FRAME_GENRE,  N_("Genre")	 },
	{ "TRSN",		   N_("Station")   },
	{ "TENC",		   N_("Encoder")   }
	};

//	id3_tag_query( fd, ID3_TAG_QUERYSIZE( 10)) != 0
	printf( "ID3 tags:\n");

	/* text information */
	for ( i = 0; i < sizeof( info) / sizeof( info[ 0]); ++i) 
	{
	union id3_field const *field;
	unsigned int nstrings, j;

	frame = id3_tag_findframe( tag, info[ i].id, 0);
	if ( frame == 0)
	  continue;

	field	= id3_frame_field( frame, 1);
	nstrings = id3_field_getnstrings( field);

	for ( j = 0; j < nstrings; ++j) 
	{
	  ucs4 = id3_field_getstrings( field, j);
	  assert(ucs4);

	  if ( strcmp( info[ i].id, ID3_FRAME_GENRE) == 0)
		  ucs4 = id3_genre_name( ucs4);

	  latin1 = id3_ucs4_latin1duplicate( ucs4);
	  if ( latin1 == 0)
		  goto fail;

	  if ( j == 0 && info[ i].label) 
	  {
	  	  //id3_utf8_encode( utf8_string, ucs4);
		  //detail( gettext( info[ i].label), 0, latin1);
		  //detail( gettext( info[ i].label), 0, utf8_string);
	  }
	  else {
	  if ( strcmp( info[ i].id, "TCOP") == 0 ||
		   strcmp( info[ i].id, "TPRO") == 0) {
		   detail( 0, "%s %s", (info[i].id[1] == 'C') ?
			   _("Copyright (C)") : _("Produced (P)"), latin1);
	  }
	  else
		  detail( 0, 0, latin1);
	  }

	  free( latin1);
	}
	}

	/* comments */

	i = 0;
	while ( ( frame = id3_tag_findframe( tag, ID3_FRAME_COMMENT, i++))) {
	ucs4 = id3_field_getstring( id3_frame_field( frame, 2));
	assert(ucs4);

	if ( *ucs4)
	  continue;

	ucs4 = id3_field_getfullstring( id3_frame_field( frame, 3));
	assert(ucs4);

	latin1 = id3_ucs4_latin1duplicate( ucs4);
	if ( latin1 == 0)
	  goto fail;

	detail(_("Comment"), 0, latin1);

	free(latin1);
	break;
	}

	if (0) {
	fail:
	error("id3", _("not enough memory to display tag"));
	}
}

//  struct id3_frame const *frame;
//
//  /* display the tag */
//
////  if (player->verbosity >= 0 || (player->options & PLAYER_OPTION_SHOWTAGSONLY))
////	show_id3(tag);
//
//  /*
//   * The following is based on information from the
//   * ID3 tag version 2.4.0 Native Frames informal standard.
//   */
//
//  /* length information */
//
//  frame = id3_tag_findframe( tag, "TLEN", 0);
//  if ( frame) 
//  {
//	  union id3_field const *field;
//	  unsigned int nstrings;
//
//	  field	= id3_frame_field( frame, 1);
//	  nstrings = id3_field_getnstrings( field);
//
//	  if ( nstrings > 0) 
//	  {
//		  id3_latin1_t *latin1;
//
//		  latin1 = id3_ucs4_latin1duplicate( id3_field_getstrings(field, 0));
//		  if ( latin1) 
//		  {
//			  signed long ms;
//
//			  /*
//		 	   * "The 'Length' frame contains the length of the audio file
//		 	   * in milliseconds, represented as a numeric string."
//		 	   */
//
//			  ms = atol( latin1);
//			  if ( ms > 0)
//			  	  printf( "MP3 frame len %ld ms\n", ms);
//	  			  //mad_timer_set( &player->stats.total_time, 0, ms, 1000);
//
//			  free( latin1);
//	  	  }
//	  }
//  }
//
//  /* relative volume adjustment information */
//  frame = id3_tag_findframe( tag, "RVA2", 0);
//  if ( frame) 
//  {
//	  id3_latin1_t const *id;
//	  id3_byte_t const *data;
//	  id3_length_t length;
//
//	  enum {
//		  CHANNEL_OTHER		 = 0x00,
//		  CHANNEL_MASTER_VOLUME = 0x01,
//		  CHANNEL_FRONT_RIGHT   = 0x02,
//		  CHANNEL_FRONT_LEFT	= 0x03,
//		  CHANNEL_BACK_RIGHT	= 0x04,
//		  CHANNEL_BACK_LEFT	 = 0x05,
//		  CHANNEL_FRONT_CENTRE  = 0x06,
//		  CHANNEL_BACK_CENTRE   = 0x07,
//		  CHANNEL_SUBWOOFER	 = 0x08
//	  };
//
//	  id   = id3_field_getlatin1( id3_frame_field( frame, 0));
//	  data = id3_field_getbinarydata( id3_frame_field( frame, 1), &length);
//
//	  assert( id && data);
//
//	  /*
//	   * "The 'identification' string is used to identify the situation
//	   * and/or device where this adjustment should apply. The following is
//	   * then repeated for every channel
//	   *
//	   *   Type of channel		 $xx
//	   *   Volume adjustment	   $xx xx
//	   *   Bits representing peak  $xx
//	   *   Peak volume			 $xx (xx ...)"
//	   */
//
//	  while ( length >= 4) 
//	  {
//		  unsigned int peak_bytes;
//
//		  peak_bytes = ( data[3] + 7) / 8;
//		  if ( 4 + peak_bytes > length)
//	  		  break;
//
//		  if ( data[0] == CHANNEL_MASTER_VOLUME) 
//		  {
//	  		  signed int voladj_fixed;
//	  		  double voladj_float;
//
//	  		  /*
//	   		   * "The volume adjustment is encoded as a fixed point decibel
//	   		   * value, 16 bit signed integer representing (adjustment*512),
//	   		   * giving +/- 64 dB with a precision of 0.001953125 dB."
//	   		   */
//
//	  		  voladj_fixed  = ( data[ 1] << 8) | ( data[ 2] << 0);
//	  		  voladj_fixed |= -( voladj_fixed & 0x8000);
//
//	  		  voladj_float  = ( double) voladj_fixed / 512;
//
//			  printf( "Relative Volume %+.1f dB adjustment (%s)\n", voladj_float, id);
//
//	  		  break;
//		  }
//
//		  data   += 4 + peak_bytes;
//		  length -= 4 + peak_bytes;
//	  }
//  }
//
//  /* Replay Gain */
//
//  frame = id3_tag_findframe( tag, "RGAD", 0);
//  if ( frame) 
//  {
//	  id3_byte_t const *data;
//	  id3_length_t length;
//
//	  data = id3_field_getbinarydata( id3_frame_field( frame, 0), &length);
//	  //assert(data);
//
//	  /*
//	   * Peak Amplitude						  $xx $xx $xx $xx
//	   * Radio Replay Gain Adjustment			$xx $xx
//	   * Audiophile Replay Gain Adjustment	   $xx $xx
//	   */
//
//	  if ( length >= 8) 
//	  {
//		  struct mad_bitptr ptr;
//		  mad_fixed_t peak;
//		  //struct rgain rgain[ 2];
//
//		  mad_bit_init( &ptr, data);
//
//		  peak = mad_bit_read( &ptr, 32) << 5;
//
////		  rgain_parse( &rgain[0], &ptr);
////		  rgain_parse( &rgain[1], &ptr);
////
////		  use_rgain(player, rgain);
//
//		  mad_bit_finish(&ptr);
//	  }
//  }
//}

static void detail( char const *label, char const *format, ...)
{
	char const spaces[] = "		 ";
	va_list args;

# define LINEWRAP  (80 - sizeof(spaces) - 2 - 2)

	if (on_same_line)
	printf("\n");

	if (label) {
	    unsigned int len;

	    len = strlen(label);
	    assert(len < sizeof(spaces));

	    fprintf(stderr, "%s%s: ", &spaces[len], label);
	}
	else
	    fprintf(stderr, "%s  ", spaces);

	va_start(args, format);

	if (format) {
	    vfprintf(stderr, format, args);
	    fputc('\n', stderr);
	}
	else {
	    char *ptr, *newline, *linebreak;

	    /* N.B. this argument must be mutable! */
	    ptr = va_arg(args, char *);

	    do {
	        newline = strchr(ptr, '\n');
	        if (newline)
	            *newline = 0;

	        if (strlen(ptr) > LINEWRAP) {
	            linebreak = ptr + LINEWRAP;

    	        while (linebreak > ptr && *linebreak != ' ')
    	            --linebreak;

    	        if (*linebreak == ' ') {
    	            if (newline)
    		            *newline = '\n';
    
    	            *(newline = linebreak) = 0;
	            }
	        }

	        fprintf(stderr, "%s\n", ptr);

	        if (newline) {
	            ptr = newline + 1;
	            fprintf(stderr, "%s  ", spaces);
	        }
	    }
	    while (newline);
	}

	va_end(args);
}

static void error( char const *id, char const *format, ...)
{
	int err;
	va_list args;

	err = errno;

	if (on_same_line)
	printf("\n");

	if (id)
	fprintf(stderr, "%s: ", id);

	va_start(args, format);

	if (*format == ':') {
	if (format[1] == 0) {
	  format = va_arg(args, char const *);
	  errno = err;
	  perror(format);
	}
	else {
	  errno = err;
	  perror(format + 1);
	}
	}
	else {
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	}

	va_end(args);
}
