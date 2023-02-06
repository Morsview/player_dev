#include <soundio/soundio.h>
#include <inttypes.h>
#include <pthread.h>
#include "ring_buffer.h"
#include "wav.h"

//#define DEBUG_PRINT
//#define FILTER

const uint32_t BUFF_SIZE = 16384/4;
const uint8_t LEFT = 0;
const uint8_t RIGHT = 1; 

pthread_t alsa_out_thread;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

ring_buffer_t *fifo;
void *output_thread(void *data)
{
    int err = 0;
//    snd_pcm_uframes_t period_size = 256;
//    snd_pcm_uframes_t buffer_size = 16384;//BUFF_SIZE;
//    snd_pcm_uframes_t period_size = 1;
//    snd_pcm_uframes_t buffer_size = 2;//BUFF_SIZE;
    
    snd_pcm_sw_params_t *sw_params;
    
    snd_pcm_sw_params_malloc ( &sw_params);
    if ( ( err = snd_pcm_sw_params_current ( ((alsa_data_t *)data)->handle, sw_params)) != 0)
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Failed snd_pcm_sw_params_current, error: %s.\n", 
                    snd_strerror( err));
        #endif
    }

    if ( ( err = snd_pcm_sw_params_set_start_threshold( ((alsa_data_t *)data)->handle, sw_params, 
                    1)) != 0)
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Failed snd_pcm_sw_params_set_start_threshold, error: %s.\n", 
                    snd_strerror( err));
        #endif
    }
    
    if ( ( err = snd_pcm_sw_params_set_avail_min(((alsa_data_t *)data)->handle, sw_params, 1)) != 0)
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Failed snd_pcm_sw_params_set_avail_min, error: %s.\n", 
                    snd_strerror( err));
        #endif
    }

    snd_pcm_sw_params( ((alsa_data_t *)data)->handle, sw_params);
    snd_pcm_sw_params_free ( sw_params);
    
    if ( ( err = snd_pcm_prepare ( ((alsa_data_t *)data)->handle)) != 0)
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Failed snd_pcm_prepare, error: %s.\n", 
                    snd_strerror( err));
        #endif
    }
    
#define sized 1024
    uint8_t *MyBuffer = malloc( 2 * sized * sizeof(uint16_t));
    memset( MyBuffer, 0,  2 * sized * sizeof(uint16_t));
    if ( snd_pcm_writei (((alsa_data_t *)data)->handle, MyBuffer, 2 * sized) > 1) 
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Initial write succeed.\n");
        #endif
    } else 
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Initial write failed.\n");
        #endif
    }
    free(MyBuffer);

    if ( ( err = snd_pcm_start(((alsa_data_t *)data)->handle)) != 0)
    {
        #ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Failed snd_pcm_start, error: %s.\n", snd_strerror( err));
        #endif
        //return pcm_handle1;//NULL;
    }

#define TRESHHOLD 16384/2
    snd_pcm_uframes_t frames;
    snd_pcm_sframes_t written;

    uint32_t fifo_out[TRESHHOLD];
    uint32_t get_len;
    ((alsa_data_t *)data)->play = 1;

    while (((alsa_data_t *)data)->play > 0)
    {
        #ifdef DEBUG_PRINT
        fprintf (stderr, "ALSA write: main loop\n");
        #endif
        pthread_mutex_lock(((alsa_data_t *)data)->fifo_mutex);
        while ( ring_buffer_size( ((alsa_data_t *)data)->fifo) < 6664) 
        {
            #ifdef DEBUG_PRINT
            fprintf (stderr, "ALSA write: NED. Wait\n");
            #endif
            pthread_cond_wait( ((alsa_data_t *)data)->data_cond, ((alsa_data_t *)data)->fifo_mutex);
        }

        get_len = ring_buffer_get( ((alsa_data_t *)data)->fifo, (uint8_t *)&fifo_out, 6664);
        #ifdef DEBUG_PRINT
        //ring_buffer_info(((alsa_data_t *)data)->fifo);
        #endif
        pthread_mutex_unlock(((alsa_data_t *)data)->fifo_mutex);
        #ifdef DEBUG_PRINT
        fprintf (stderr, "ALSA write: FIFO poped %d bytes\n", get_len);
        #endif

        if ( ( err = snd_pcm_wait ( ( snd_pcm_t *)( ( alsa_data_t *)data)->handle, 1000)) < 0) {
            if ( errno == 11) break;
            fprintf ( stderr, "poll failed (%s) code %d\n", strerror( errno), errno);
            continue;
        }

        frames = snd_pcm_bytes_to_frames( ( snd_pcm_t *)( ( alsa_data_t *)data)->handle, get_len);
        while ( frames > 0)
        {
            written = snd_pcm_writei( ( snd_pcm_t *)( ( alsa_data_t *)data)->handle, &fifo_out, frames);
            if ( written < 0)
            {
#               ifdef DEBUG_PRINT
                fprintf( stderr, "ALSA write: Failed snd_pcm_writei, error: %s.\n", 
                        snd_strerror( written));
#               endif
                written = snd_pcm_recover(((alsa_data_t *)data)->handle, (int)written, 0);
#               ifdef DEBUG_PRINT
                fprintf( stderr, "ALSA write: Failed snd_pcm_recover error: %s EXIT THREAD!!!.\n", 
                        snd_strerror( written));
#               endif
            }
            else
            {
                frames -= written;
                #ifdef DEBUG_PRINT
                fprintf (stderr, "ALSA write: wrote %d frames, remains %d framess\n", written, frames);
                #endif
            }
        }
        //usleep(200);
    }
    #ifdef DEBUG_PRINT
    fprintf (stdout, "ALSA write: EXIT THREAD!!!\n");
    #endif
    return NULL;
}
/*********** FILTER **********/
#ifdef FILTER
typedef struct list list_t;
struct list
{
    char *line;
    list_t *prev;
    list_t *post;
}; 
float *coeff;
int filter_order;
float *prev_init;
#endif
/*********** FILTER END**********/

/**
 * ALSA initialisation 
 **/
alsa_data_t *init_alsa( const char *snd_device)
{
    /*********** FILTER **********/
    #ifdef FILTER

    FILE *coeff_file;    
    char *line = NULL;
    size_t len = 0;
    //if ( (coeff_file = fopen( "coeff.2", "r")) == NULL)
    if ( (coeff_file = fopen( "coeff.fft", "r")) == NULL)
        printf( "error opening coeff file\n");
    
    filter_order = 0;
    list_t *pre = NULL;
    list_t *entry = NULL;
    while ( getline( &line, &len, coeff_file) != -1)
    {
        entry = ( list_t *)malloc( sizeof( list_t));
        entry->line = ( char *)malloc( sizeof( char) * len);
        memcpy( entry->line, line, len);
        entry->prev = pre;
        pre = entry;
        filter_order++;
    }

    coeff = ( float *)malloc( sizeof( float) * filter_order);
    prev_init = ( float *)malloc( sizeof( float) * filter_order);

    for ( int i = filter_order - 1; i >= 0; i--)
    {
        coeff[ i] = atof( entry->line);
        entry = entry->prev;
        #ifdef DEBUG_PRINT
        //fprintf( stderr, "%f\n", coeff[ i]);
        #endif
        prev_init[ i] = 0.0;
    }
    #endif
    /*********** FILTER END**********/
    int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "error connecting: %s\n", soundio_strerror(err));
        return 1;
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found\n");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    outstream->format = SoundIoFormatFloat32NE;
    outstream->write_callback = write_callback;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return 1;
    }

    for (;;)
        soundio_wait_events(soundio);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    /* This holds the error code returned */
    int err;
    
    /* Our device handle */
    snd_pcm_t *pcm_handle = NULL;
    
    /* The device name */
    const char *device_name = "default";
    if ( snd_device != NULL)
    	device_name = snd_device;
    else
    	device_name = "default";
    //const char *device_name = "hw:3,0"; //DMX 6fire
    
    /* Open the device */
    err = snd_pcm_open ( &pcm_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0);
    
    /* Error check */
    if ( err < 0) 
    {
        //#ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Can not open an audio device %s (%s)\n", 
            device_name, snd_strerror( err));
        //#endif
        pcm_handle = NULL;
        exit( -1);
    }
    snd_pcm_hw_params_t *hw_params;
    unsigned int rrate = 44100;
    
    snd_pcm_hw_params_malloc ( &hw_params);
    snd_pcm_hw_params_any ( pcm_handle, hw_params);
    snd_pcm_hw_params_set_access ( pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format ( pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near ( pcm_handle, hw_params, &rrate, NULL);
    snd_pcm_hw_params_set_channels ( pcm_handle, hw_params, 2);
    #ifdef DEBUG_PRINT
    fprintf( stderr, "ALSA backend: sampling rate %dHz\n", rrate);
    #endif

    /* These values are pretty small, might be useful in
       situations where latency is a dirty word. */
    snd_pcm_uframes_t buffer_size = 4096;//32 * rrate;//BUFF_SIZE;
    snd_pcm_uframes_t period_size = buffer_size / 4;
    
    snd_pcm_hw_params_set_buffer_size_near ( pcm_handle, hw_params, &buffer_size);
    snd_pcm_hw_params_set_period_size_near ( pcm_handle, hw_params, &period_size, NULL);
    #ifdef DEBUG_PRINT
    fprintf( stderr, "ALSA backend: period size %d bytes, byffer_size %d bytes\n", 
            ( uint32_t)period_size, ( uint32_t)buffer_size);
    #endif

    snd_pcm_hw_params ( pcm_handle, hw_params);
    snd_pcm_hw_params_free ( hw_params);

    fifo = ( ring_buffer_t *)malloc( sizeof( ring_buffer_t));
    init_ring_buffer( fifo, 16384 * 16);

    alsa_data_t *alsa_data;
    alsa_data = ( alsa_data_t*)malloc( sizeof( alsa_data_t));

    alsa_data->handle = pcm_handle;
    alsa_data->fifo_mutex = &buffer_mutex;
    alsa_data->data_cond = &cond_var;
    alsa_data->fifo = fifo;
    alsa_data->alsa_out_thread = &alsa_out_thread;
    alsa_data->play = 1;

    if ( pthread_create( &alsa_out_thread, NULL, output_thread, (void *)alsa_data))
    {
#ifdef DEBUG_PRINT
        fprintf( stderr, "ALSA backend: Can not create outputhread\n");
#endif
        return NULL;
    }
//    if( pthread_join(alsa_out_thread, NULL)) 
//    {
//#ifdef DEBUG_PRINT
//        fprintf(stderr, "Error joining thread\n");
//#endif
//        return NULL;    
//    }

    return alsa_data;    
}

int alsa_close( void *alsa_data)
{
    return snd_pcm_close( ( snd_pcm_t *)( ( alsa_data_t *)data)->handle);
}

/**
 * Fuction writes buffer to alsa output 
 **/
//static uint32_t iter = 0;
int write_to_alsa( alsa_data_t *alsa_data, uint32_t *buffer, unsigned long length)
{
    unsigned long remained = length * 4;
    /*********** FILTER **********/
#   ifdef FILTER
    int filter_buffer[ filter_order + length];
    memcpy( filter_buffer, prev_init, sizeof( uint32_t) * filter_order);
    memcpy( filter_buffer + filter_order, buffer, sizeof( uint32_t) * length);
    split_ch_t *input_buffer = ( split_ch_t *)filter_buffer;
    split_ch_t *out_buffer = ( split_ch_t *)buffer;
    memcpy( prev_init,  buffer + ( length - filter_order), sizeof( uint32_t) * filter_order);   

    for ( int i = filter_order; i < length + filter_order; i++)
    {
        float temp_left = 0.0;
        float temp_right = 0.0;
        for ( int j = 0; j < filter_order; j++)
        {
            temp_left += ( float)input_buffer[ i - j].ch[ LEFT] * coeff[ j];
            temp_right += ( float)input_buffer[ i - j].ch[ RIGHT] * coeff[ j];
        }
        out_buffer[ i - filter_order].ch[ LEFT] = ( uint16_t)( temp_left);
        out_buffer[ i - filter_order].ch[ RIGHT] = ( uint16_t)( temp_right);
    }
#   endif
    /*********** FILTER END**********/
    while ( remained > 0)
    {
        //int err = 0;
        int wr = 0;
        #ifdef DEBUG_PRINT
        //fprintf( stderr, "ALSA backend: lock mutex.\n"); 
        #endif
        pthread_mutex_lock( alsa_data->fifo_mutex);
        wr = ring_buffer_push( ( ( alsa_data_t *)alsa_data)->fifo, 
                               ( ( uint8_t *)buffer) + ( length * 4 - remained) , remained);
        #ifdef DEBUG_PRINT
        ring_buffer_info( ( ( alsa_data_t *)alsa_data)->fifo);
        //fprintf( stderr, "ALSA backend: unlock mutex.\n"); 
        #endif

        pthread_mutex_unlock( alsa_data->fifo_mutex);
        #ifdef DEBUG_PRINT
        //fprintf( stderr, "ALSA backend: send signal.\n"); 
        #endif
        pthread_cond_broadcast( alsa_data->data_cond);
        remained -= wr;
        #ifdef DEBUG_PRINT
        if ( wr != 0)
        fprintf( stderr, "ALSA backend: Fill the FIFO with new data. Wrote %d bytes. Remained %d\n", 
                    wr, remained);
        #endif
        if ( remained > 0)
        {
            usleep( 1000);
        }
    }

    return 0;
}

