/**
  * Ring buffer header file.
  **/
#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include<inttypes.h>
#include <pthread.h>

typedef struct ring_buffer 
{
    uint8_t  *buffer; 
    uint64_t  buffer_head;
    uint64_t  buffer_tail;
    uint64_t  buffer_size;
    uint64_t  actual_size;
    pthread_mutex_t *fifo_mutex;
} ring_buffer_t;

/**
  * Initiating specified ring buffer 
  **/
int init_ring_buffer( ring_buffer_t *rbuffer, uint64_t init_len);

/**
  * Push data(input) with length( input_lendth) to specified ring buffer(rbuffer).
  * Function returns num of pushed bytes.
  **/
uint64_t ring_buffer_push( ring_buffer_t *rbuffer, uint8_t *input, uint64_t input_len);

/**
  * Get data(output) with desired_len'ght from the rbuffer. 
  * Function returs count of bytes moved to the output.
  **/
uint64_t ring_buffer_get( ring_buffer_t *rbuffer, uint8_t *output, uint64_t desired_len);

/**
  * Function prints ring buffer statistics
  **/
void ring_buffer_info( ring_buffer_t *rbuffer);

/**
  * Function returns ring buffer size
  **/
uint64_t ring_buffer_size( ring_buffer_t *rbuffer);

/**
  * Freeng specified ring buffer. 
  **/
void ring_buffer_free( ring_buffer_t *rbuffer);
#endif /* __RING_BUFFER_H__ */
