#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "wav.h"
//#include <alsa/asoundlib.h>
//#include "alsa.h"
#include "constants.h"

// TODO 
int write_to_alsa( void *alsa_data, uint32_t *buffer, unsigned long length);

FILE *opn_wav( char *filename, unsigned long *length)
{
    FILE *wav = fopen( filename, "rb");
    wav_header_t header;

    if ( wav == NULL)
	{
        fprintf( stderr, "Can't open input file %s", filename);
        exit( 0);
    }

    /* read header */
    if ( fread( &header, sizeof(header), 1, wav) < 1 )
    {
        fprintf( stderr, "Can't read file header\n");
        exit( 0);
    }
    if (    header.id[ 0] != 'R'
         || header.id[ 1] != 'I'
         || header.id[ 2] != 'F'
         || header.id[ 3] != 'F') 
	{
        fprintf( stderr, "ERROR: Not wav format\n");
        exit( 0);
    }

    fprintf( stderr, "wav format\n");

	*length = ( header.totallength / 4);
    printf( "id: %s\nlength: %i\nformat: %i\nchannels: %i\nfrequency: %i\nbytes per second: %i\nbytes per capture: %i\nbits per sample: %i\nbytes in data: %i\n",
            header.id, header.totallength, header.format,
            header.channels, header.frequency, header.bytes_per_second,
            header.bytes_by_capture, header.bits_per_sample, header.bytes_in_data);

    if ( header.frequency != 44100) 
    {
        printf( "Wave Error: Sampling frequency differs 44100KHz\n");
        exit(-13);
    }

	return wav;
}
/**
 * Main function for WAV playback
 **/ 
void play_wav( char *filename, void *alsa_data)
{
	FILE *wav;
	int i = 0;
	unsigned long length;
	uint32_t time = 0;
	uint32_t buffer[ BUFF_SIZE/8];

	wav = opn_wav( filename, &length);
	if ( wav == NULL)
	{
		fprintf( stderr, "Can't open WAV file\n");
	}

	printf( "time:  00:00");
    for ( i = 0; i < length; i++) 
	{
        if ( fread( buffer, sizeof( buffer[0]), BUFF_SIZE/8, wav) < 1)
	    {
    	    fprintf( stderr, "End of file\n");
        	break;
   		}

		if ( ( write_to_alsa( alsa_data, buffer, BUFF_SIZE/8)) == -1)
			fprintf( stderr, "Wave: write to alsa failed.\n");

        //fprintf( stderr, "write_to_alsa %lu bytes\n", BUFF_SIZE);
		time += BUFF_SIZE/8;
		printf( "\b\b\b\b\b%2i:%2i", time / 2646000, ( time % 2646000) / 44100);
    }
	fclose( wav);
}
