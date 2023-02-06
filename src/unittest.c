#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include "ring_buffer.h"

int main()
{
    ring_buffer_t *fifo;
    fifo = ( ring_buffer_t *)malloc( sizeof( ring_buffer_t));

    init_ring_buffer( fifo, 2 * 4096);

    uint8_t buff[ 4096];
    uint8_t buff2[ 4096];
    for ( int i = 0; i < 4096; i++)
    {
        buff[ i] = rand() % 127;
        buff2[ i] = rand() % 127;
    }

    ring_buffer_info( fifo);
    ring_buffer_push( fifo, buff, 4096);
    uint8_t out[ 4096];
    ring_buffer_get( fifo, out, 4096);

    if ( memcmp( buff, out, 4096) == 0) 
    {
        printf( "Unittest #1: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else
    {
        printf( "Unittest #1: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }

    ring_buffer_push( fifo, buff2, 4096);
    ring_buffer_info( fifo);

    ring_buffer_get( fifo, out, 1024);

    if ( memcmp( buff2, out, 1024) == 0)
    {
        printf( "Unittest #2: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #2: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }
    
    ring_buffer_get( fifo, out, 2048);

    if ( memcmp( buff2 + 1024, out, 2048) == 0)
    {
        printf( "Unittest #3: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #3: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }

    ring_buffer_push( fifo, buff, 4096);
    ring_buffer_info( fifo);
    ring_buffer_get( fifo, out, 1024);

    if ( memcmp( buff2 + 1024 + 2048, out, 1024) == 0)
    {
        printf( "Unittest #4: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #4: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }
    
    ring_buffer_push( fifo, buff2, 3000);
    ring_buffer_info( fifo);

    ring_buffer_get( fifo, out, 4096);
    if ( memcmp( buff, out, 4096) == 0)
    {
        printf( "Unittest #5: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #5: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }
    
    ring_buffer_push( fifo, buff2 + 3000, 1000);
    ring_buffer_info( fifo);
    ring_buffer_push( fifo, buff, 1023);
    ring_buffer_info( fifo);
    ring_buffer_push( fifo, buff + 1023, 1023);
    ring_buffer_info( fifo);
    ring_buffer_push( fifo, buff + 1023 + 1023, 1000);
    ring_buffer_info( fifo);

    ring_buffer_get( fifo, out, 4000);
    if ( memcmp( buff2, out, 4000) == 0)
    {
        printf( "Unittest #6: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #6: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }
    ring_buffer_get( fifo, out, 3046);
    if ( memcmp( buff, out, 3046) == 0)
    {
        printf( "Unittest #7: FIFO works.\n");
        ring_buffer_info( fifo);
    }
    else 
    {
        printf( "Unittest #7: FIFO test failed.\n");
        ring_buffer_info( fifo);
    }
    
    return 0;
}
