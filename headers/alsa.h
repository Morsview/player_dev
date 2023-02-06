/**
 * Header file for asing ALSA API
 **/

#ifndef __ALSA_H__
#define __ALSA_H__

#include <pthread.h>
#include "ring_buffer.h"

typedef struct {
    void            *handle;
    pthread_mutex_t *fifo_mutex;
    pthread_cond_t  *data_cond;
    ring_buffer_t   *fifo;
    pthread_t       *alsa_out_thread;
    int             play;
} alsa_data_t;

void *init_alsa( const char *snd_device);
int alsa_close( void *alsa_data);
int write_to_alsa( void *alsa_data, uint32_t *buffer, unsigned long length);
#endif /* __ALSA_H__ */
