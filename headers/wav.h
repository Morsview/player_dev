#ifndef __WAV_H__
#define __WAV_H__

//#include <alsa/asoundlib.h>
//#include "alsa.h"

FILE *opn_wav( char *filename, unsigned long *length);
void play_wav( char *filename, void *alsa_data);

typedef struct wavfile
{
    char        id[ 4];          // should always contain "RIFF"
    int     totallength;    	 // total file length minus 8
    char        wavefmt[ 8];     // should be "WAVEfmt "
    int     format;         	 // 16 for PCM format
    short     pcm;            	 // 1 for PCM format
    short     channels;       	 // channels
    int     frequency;      	 // sampling frequency
    int     bytes_per_second;
    short     bytes_by_capture;
    short     bits_per_sample;
    char        data[ 4];        // should always contain "data"
    int     bytes_in_data;
} wav_header_t;

typedef union 
{
    int frame;
    short ch[ 1];
} split_ch_t;

#endif /* __WAV_H__ */
