#pragma once
#include <FLAC/stream_decoder.h>

FLAC__StreamDecoderWriteStatus write_callback( const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
void error_callback( const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
void metadata_callback( const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
int play_flac( char *filename, alsa_data_t *alsa_data);
