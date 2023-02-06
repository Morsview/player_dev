/**
 * Equalizer header file
 **/

#pragma once
#include "wav.h"

split_ch *equalize( split_ch *);
void eq_init (void);
void eq_cleanup (void);
void eq_set_format (int new_channels, int new_rate);
void eq_filter (float * data, int samples);
