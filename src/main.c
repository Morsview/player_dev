/*
 * Simple FLAC/MP3/WAV player.
 */

//#include <alsa/asoundlib.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include "wav.h"
#include "alsa.h"
#include "mp3.h"
#include "flac.h"
#include "constants.h"

const uint32_t BUFF_SIZE = 16384/4;
const uint8_t LEFT = 0;
const uint8_t RIGHT = 1; 

const char* shortopts = "D:h";
static const struct option longopts[] = {
	{
	  .name = "device",
	  .has_arg = required_argument,
	  .val = 'D',
	},
	{
	  .name = "help",
	  .has_arg = no_argument,
	  .val = 'h',
	},
	{},
};

static const char* help = 
	"[OPTION] [FILE .mp3|.flac|.wav]\n\n"
	"-h, --help             print help\n"
	"-D, --device=NAME      select PCM device by name\n";

int main( int argc, char *argv[])
{
	char *filename = NULL;
	char *snd_device = NULL;
	void *alsa_data;
	int file = 0;

	for ( ;;) {
		int opt = getopt_long( argc, argv, shortopts, longopts, NULL);

		if ( opt == -1)
			break;

		switch ( opt) {
			case 'D':
				printf( "Open sound device: %s\n", optarg);
				snd_device = optarg;
				break;
			case 'h':
				printf( "%s %s", argv[ 0], help);
				exit( 0);
				break;
			default:
				break;
		}
	}

	/* Check for the one non-optional argument */
	if ( optind < argc && ( argc - optind) == 1) {
		filename = argv[ optind];
	} else {
		fprintf( stderr, "Please specify filename to open.\n");
		fprintf( stdout, "%s %s", argv[ 0], help);
		exit( -1);
	}

	if ( ( alsa_data = init_alsa( snd_device)) == NULL)
	exit( -5);

	if ( strstr( filename, "mp3") != NULL) 
	{
		printf( "Playing MP3: %s\n", filename);
		if ( ( file = open( filename, O_RDONLY)) == -1)
		{
			fprintf( stderr, "Failed to open MP3 file\n");
			return -1;
		}
		
		struct stat stat;
		if ( fstat( file, &stat) == -1 || stat.st_size == 0)
			return -2;

		play_mp3( alsa_data, file, stat.st_size);

	} else if ( strstr( filename, ".wav") != NULL)
	{
		printf( "Playing WAV: %s\n", filename);
		play_wav( filename, alsa_data);
	} else if ( strstr( filename, ".flac") != NULL)
	{
		printf( "Playing FLAC: %s\n", filename);
		play_flac( filename, alsa_data);	
	} else
	{
		fprintf( stderr, "\nIncorrect format.\n");
		exit( -2);
	}

	alsa_close( alsa_data);
	return 0;
}
