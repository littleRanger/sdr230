#include <stdio.h>
#include <stdlib.h>
// #include <itpp/itbase.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <hackrf.h>
// #include "test.h"
#include "header.h"
// #include "cap.h"

// using namespace itpp;
// using namespace std;

#define PI 3.141592654

#define DEBUG 1
// #define DEBUG_

#define FSK_DEFAULT_MSG "010100100101011011110011"
#define ISREPEAT 0

typedef enum
{
    TRANSCEIVER_MODE_OFF = 0,
    TRANSCEIVER_MODE_RX = 1,
    TRANSCEIVER_MODE_TX = 2,
    TRANSCEIVER_MODE_SS = 3,
} transceiver_mode_t;

typedef enum
{
    HW_SYNC_MODE_OFF = 0,
    HW_SYNC_MODE_ON = 1,
} hw_sync_mode_t;

/********global************/
volatile uint32_t byte_count = 0;

int limit_num_samples = -1;
uint64_t samples_to_xfer = 0;
size_t bytes_to_xfer = 0;

/**************************/

// Open the HACKRF device
// int config_hackrf(const hackrf_device *dev, const int16 *gain)
// {
//     unsigned int lna_gain = 40; // default value
//     unsigned int vga_gain = 40; // default value
//     // if (gain != -99)
//     //     vga_gain = (gain / 2) * 2;

//     int result = hackrf_init();
//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_init() failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }

//     result = hackrf_open(&dev);
//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_open() failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }

//     double sampling_rate = SAMP_RATE;
//     // Sampling frequency
//     result = hackrf_set_sample_rate_manual(dev, sampling_rate, 1);
//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }

//     // Need to make more study in the future. temperily set it 0.
//     result = hackrf_set_baseband_filter_bandwidth(dev, 0);
//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }

//     result = hackrf_set_vga_gain(dev, vga_gain);
//     result |= hackrf_set_lna_gain(dev, lna_gain);

//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_set_vga_gain hackrf_set_lna_gain failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }

//     // Center frequency
//     result = hackrf_set_freq(dev, FREQ_CENTRE);
//     if (result != HACKRF_SUCCESS)
//     {
//         printf("config_hackrf hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((result)), result);
//         // ABORT(-1);
//         return (result);
//     }
//     return (result);
// }

int8_t *txbuffer;
// int8_t *txbufferI;
// int8_t *txbufferQ;


// int bufferOffset;
char *bits;
int msg_size = 0;

//give data to hackrfrf
int tx_callback(hackrf_transfer *transfer)
{
    int i;    
    // printf("a\n");
    // How much to do?
    size_t count = transfer->valid_length;
    // printf("%d,%d\n",count,msg_size);
    if(msg_size == 0)
        return 0;
    if(count > msg_size)
    {
        printf("count > msg\n");
        for(i=0;i<msg_size;i++)
        {
            transfer->buffer[i] = txbuffer[i];
            // printf("%d ", txbuffer[i]);
        }
        msg_size = 0;
    }
    else
    {
        // printf("count < msg\n");

        // msg_size = msg_size-count;
        for(i=0;i<count;i++)
        {
            transfer->buffer[i] = txbuffer[i];
            // printf("%d ", txbuffer[i]);

        }
    }
        // memcpy(transfer->buffer, txbuffer, msg_size*sizeof(int8_t));
    // else
    //     {
    //         memcpy(transfer->buffer, txbuffer, count*sizeof(int8_t));
    //         msg_size  = msg_size - count;
    //         txbuffer = txbuffer + count;
    //     }

    // if(count > msg_size)
    // int i = 0;
    // bufferOffset = 0;
    // while ((i < count) && (bufferOffset < msg_size))
    // {
        //
        // (transfer->buffer)[i++] = txbufferI[bufferOffset]; // I
        // (transfer->buffer)[i++] = txbufferQ[bufferOffset]; // Q
        // bufferOffset++;
        // printf("%5d%5d,", txbufferI[i - 2], txbufferQ[i - 1]);
    // }
    return 0;
}

int tx(hackrf_device *dev)
{

    unsigned int lna_gain = 0;
    unsigned int vga_gain = 20;
    // if (gain != -99)
    //     vga_gain = (gain / 2) * 2;

    int result = hackrf_init();
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_init() failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    result = hackrf_open(&dev);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_open() failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    double sampling_rate = SAMP_RATE;
    // Sampling frequency
    result = hackrf_set_sample_rate_manual(dev, sampling_rate, 1);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    // Need to make more study in the future. temperily set it 0.
    result = hackrf_set_baseband_filter_bandwidth(dev, 0);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    result = hackrf_set_vga_gain(dev, vga_gain);
    result |= hackrf_set_lna_gain(dev, lna_gain);

    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_set_vga_gain hackrf_set_lna_gain failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    // Center frequency
    result = hackrf_set_freq(dev, FREQ_CENTRE);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((result)), result);
        // ABORT(-1);
        return (result);
    }

    bits = NULL;
    // if (bits == NULL)
    //     bits = strdup(FSK_DEFAULT_MSG);

    FILE *fp = NULL;
    int8_t *in = NULL;
    fp = fopen("/root/work/hackrf/dataxx_1times", "rb");
    if (fp == NULL)
    {
        printf("cannot open file.\n");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    if(size>0)
    {
        in = (int8_t*)calloc(size, sizeof(int8_t));
    }

    fread(in, size, sizeof(int8_t), fp);
    fclose(fp);

    printf("%d\n",size);

    txbuffer = (int8_t*)calloc(800 * size, sizeof(int8_t));
    if(txbuffer==NULL)
        printf("buffer malloc failed!\n");
    // txbufferI = malloc(400*size*sizeof(int8_t));
    // txbufferQ = malloc(400*size*sizeof(int8_t));
    int tmp;
    int i;
    msg_size = 800 * size;
    for (int j = 0; j < 400*size; j++)
    {
               tmp = j%200;
               i = j/400;
            //    txbuffer[2*j]   = (int8_t)(in[i]*127.0*cos(tmp*2.0*PI/200.0) + (1-in[i]) * 127.0 * sin(tmp*2.0*PI/200.0) );
            txbuffer[2*j]   = (int8_t)(in[i]*127.0*cos(tmp*2.0*PI/200.0) + (1-in[i]) * 127.0 * cos(tmp*2.0*PI/200.0) );

            //    txbuffer[2*j+1] = (int8_t)(in[i]*127.0*sin(tmp*2.0*PI/200.0) - (1-in[i]) * 127.0 * cos(tmp*2.0*PI/200.0) );
               txbuffer[2*j+1] = (int8_t)(in[i]*127.0*sin(tmp*2.0*PI/200.0) - (1-in[i]) * 127.0 * sin(tmp*2.0*PI/200.0) );
            //    if(j<10)
            //    printf("%5d%5d\n",txbuffer[2*j],txbuffer[2*j+1]);

            //    txbufferI[tmp] = (int8_t)(in[j]*127.0*cos(tmp*2.0*PI/200.0) + in_opp[j]*127.0*sin(tmp*2.0*PI/200.0) );
            //    txbufferQ[tmp] = (int8_t)(in[j]*127.0*sin(tmp*2.0*PI/200.0) - in_opp[j]*127.0*cos(tmp*2.0*PI/200.0) );
            //    printf("%lf\n",127*cos(tmp*2*PI/200));
            //    printf("%5d%5d,",txbufferI[tmp],txbufferQ[tmp]);
            // tmp++;
        
    }
    // printf("%d\n",400*size*sizeof(int8_t)/1024);

    struct timeval time_start;
    gettimeofday(&time_start, NULL);

    // #ifdef DEBUG
    printf("time_start:%ld\n", 1000000 * time_start.tv_sec + time_start.tv_usec);

    // #endif

    //   result = hackrf_stop_tx(dev);
    //   if( result != HACKRF_SUCCESS ) {
    //     printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((result)), result);
    //     exit(-1);
    //   }
    printf("end\n");

    result = hackrf_start_tx(dev, tx_callback, NULL);
    if (result != HACKRF_SUCCESS)
    {
        fprintf(stderr, "hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
        exit(-1);
    }
    // printf("2\n");
    // while( (hackrf_is_streaming(device) == HACKRF_TRUE) &&(do_exit == false) )
    while ((hackrf_is_streaming(dev) == HACKRF_TRUE))
    {
        struct timeval time_now;
        gettimeofday(&time_now, NULL);

        if ((1000000 * (time_now.tv_sec - time_start.tv_sec) + time_now.tv_usec - time_start.tv_usec) > 30000000)
        {
#ifdef DEBUG
            printf("time_now : %ld\n", 1000000*time_now.tv_sec + time_now.tv_usec);
            printf("time_gap : %ld\n", 1000000 * (time_now.tv_sec - time_start.tv_sec) + time_now.tv_usec - time_start.tv_usec);
#endif
            break;
        }
    }

    result = hackrf_stop_tx(dev);
    // printf("b\n");

    if (result != HACKRF_SUCCESS)
    {
        // fprintf(stderr, "hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name(result), result);
        fprintf(stderr, "hackrf_stop_rx() failed: (%d)\n", result);
    }
    else
    {
        fprintf(stderr, "hackrf_stop_tx() done\n");
    }
    free(in);
    return result;
}

int main()
{
    hackrf_device *hackrf_dev = NULL;
    int result = 0;

    // if (config_hackrf(hackrf_dev, 0) == 0)
    // {
    //     printf("HACKRF device FOUND!\n");
    // }
    // else
    // {
    //     printf("HACKRF device not FOUND!\n");
    //     //     ABORT(-1);
    // }

    //testing
    tx(hackrf_dev);
    
    
    result = hackrf_close(hackrf_dev);
    if (result != HACKRF_SUCCESS)
    {
        // fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
        fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
    }
    else
    {
        fprintf(stderr, "hackrf_close() done\n");
    }
    hackrf_exit();
    fprintf(stderr, "hackrf_exit() done\n");


        free(txbuffer);
}
