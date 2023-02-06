/**
  * Ring buffer implenentation for async output.
  **/
#include<inttypes.h>
#include<stdlib.h>
#include<string.h>
#include"ring_buffer.h"
#include<stdio.h>

//#define DEBUG_PRINT 

static pthread_mutex_t buffer_mutex;
/**
  * Initiating specified ring buffer 
  **/
int init_ring_buffer( ring_buffer_t *rbuffer, uint64_t init_len)
{
    rbuffer->buffer = (uint8_t *)malloc( init_len);
    if ( rbuffer->buffer == NULL)
    {
#ifdef DEBUG_PRINT
        fprintf( stderr, "FIFO: Error: can not alocate buffer %d bytes.\n", init_len);
#endif
        return -1;
    }
    rbuffer->buffer_head = 0;
    rbuffer->buffer_tail = 0;
    rbuffer->actual_size = 0;
    rbuffer->buffer_size = init_len;
    #ifdef DEBUG_PRINT
    fprintf( stderr, "FIFO: Initiated %d bytes.\n", init_len);
    #endif

    rbuffer->fifo_mutex = &buffer_mutex;
    return init_len;
}

/**
  * Push data(input) with length( input_lendth) to specified ring buffer(rbuffer).
  * Function returns num of pushed bytes.
  **/
uint64_t ring_buffer_push( ring_buffer_t *rbuffer, uint8_t *input, uint64_t input_len)
{
    const uint8_t *p = input;

    #ifdef DEBUG_PRINT
    fprintf( stderr, "FIFO: push %ld bytes.\n", input_len);
    #endif
    if ( ( rbuffer->buffer_size - rbuffer->actual_size) < input_len) {
#ifdef DEBUG_PRINT
            fprintf( stderr, "FIFO: NES. Exit.\n");
#endif
        return 0;
    }

    for ( uint64_t i = 0; i < input_len; i++) 
    {
        if ( ( rbuffer->buffer_head + 1 == rbuffer->buffer_tail) ||
                (( rbuffer->buffer_head + 1 == rbuffer->buffer_size) && 
                 ( rbuffer->buffer_tail == 0)))
        {
            //pthread_mutex_lock( rbuffer->fifo_mutex);
            rbuffer->actual_size += i;
            //pthread_mutex_unlock( rbuffer->fifo_mutex);
#ifdef DEBUG_PRINT
            fprintf( stderr, "FIFO: wrote %ld bytes. NES. Exit\n", i);
#endif
            return i;
        }
        else
        {
            //pthread_mutex_lock( rbuffer->fifo_mutex);
            rbuffer->buffer[ rbuffer->buffer_head] = *p++;
            rbuffer->buffer_head++;
            if ( rbuffer->buffer_head == rbuffer->buffer_size)
            {
                rbuffer->buffer_head = 0;
            }
            //pthread_mutex_unlock( rbuffer->fifo_mutex);
        }
    }
    //pthread_mutex_lock( rbuffer->fifo_mutex);
    rbuffer->actual_size += input_len;
    //pthread_mutex_unlock( rbuffer->fifo_mutex);
#ifdef DEBUG_PRINT
    fprintf( stderr, "FIFO: wrote %ld bytes. ALL.\n", input_len);
#endif
    return input_len;
}

/**
  * Get data(output) with desired_len'ght from the rbuffer. 
  * Function returs count of bytes moved to the output.
  **/
uint64_t ring_buffer_get( ring_buffer_t *rbuffer, uint8_t *output, uint64_t desired_len)
{
    uint8_t *p = output;
#ifdef DEBUG_PRINT
    fprintf( stderr, "FIFO: POP %ld bytes.\n", desired_len);
#endif

    if ( rbuffer->actual_size < desired_len) {
#ifdef DEBUG_PRINT
            fprintf( stderr, "FIFO: NED. Exit.\n");
#endif
        return 0;
    }

    for ( uint64_t i = 0; i < desired_len; i++)
    {
        if ( rbuffer->buffer_tail != rbuffer->buffer_head) /* see if any data is available */
        {
            //pthread_mutex_lock( rbuffer->fifo_mutex);
            *p++ = rbuffer->buffer[ rbuffer->buffer_tail]; /* grab a byte from the buffer */
            rbuffer->buffer_tail++;                        /* increment the tail */
            if ( rbuffer->buffer_tail == rbuffer->buffer_size) /* check for wrap-around */
            {
                rbuffer->buffer_tail = 0;
            }
            //pthread_mutex_unlock( rbuffer->fifo_mutex);
        }
        else
        {
            //pthread_mutex_lock( rbuffer->fifo_mutex);
            rbuffer->actual_size -= desired_len;
            //pthread_mutex_unlock( rbuffer->fifo_mutex);
#ifdef DEBUG_PRINT
            fprintf( stderr, "FIFO: got %ld bytes. NED. Exit.\n", i);
#endif
            return i;
        }
    }

    //pthread_mutex_lock( rbuffer->fifo_mutex);
    rbuffer->actual_size -= desired_len;
    //pthread_mutex_unlock( rbuffer->fifo_mutex);
#ifdef DEBUG_PRINT
    fprintf( stderr, "FIFO: got %ld bytes. ALL.\n", desired_len);
#endif
    return desired_len;
}

/**
  * Function prints ring buffer statistics
  **/
void ring_buffer_info( ring_buffer_t *rbuffer)
{
    fprintf( stderr, "FIFO: Size %7lu, Head %ld, Tail %ld\n", 
                        rbuffer->actual_size,
                        rbuffer->buffer_head,
                        rbuffer->buffer_tail);
}

/**
  * Function returns ring buffer size
  **/
uint64_t ring_buffer_size( ring_buffer_t *rbuffer)
{
    return rbuffer->actual_size;
}

/**
  * Freeng specified ring buffer. 
  **/
void ring_buffer_free( ring_buffer_t *rbuffer)
{
    free( rbuffer->buffer);
}
