#include <stdio.h>
#include <stdlib.h>
#include <itpp/itbase.h>
#include <sys/time.h>
#include <unistd.h>
#include <hackrf.h>
#include "test.h"
#include "header.h"
// #include "cap.h"

using namespace itpp;
using namespace std;

#define DEBUG
// #define DEBUG_

typedef enum {
        TRANSCEIVER_MODE_OFF = 0,
        TRANSCEIVER_MODE_RX = 1,
        TRANSCEIVER_MODE_TX = 2,
        TRANSCEIVER_MODE_SS = 3,
} transceiver_mode_t;

typedef enum {
	HW_SYNC_MODE_OFF = 0,
	HW_SYNC_MODE_ON = 1,
} hw_sync_mode_t;

/********global************/
FILE* fd = NULL;
volatile uint32_t byte_count = 0;
uint64_t stream_size = 0;
uint32_t stream_head = 0;
uint32_t stream_tail = 0;
uint32_t stream_drop = 0;
uint8_t *stream_buf = NULL;

bool limit_num_samples = false;
uint64_t samples_to_xfer = 0;
size_t bytes_to_xfer = 0;
/**************************/


// Open the HACKRF device
int config_hackrf( hackrf_device * & dev, const int16 & gain)
{
    unsigned int lna_gain=40; // default value
    unsigned int vga_gain=40; // default value
    if (gain!=-9999)
        vga_gain = (gain/2)*2;

    int result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_init() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
    // ABORT(-1);
        return(result);
	}

	result = hackrf_open(&dev);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_open() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
    // ABORT(-1);
        return(result);
	}

    double sampling_rate = SAMP_RATE;
    // Sampling frequency
    result = hackrf_set_sample_rate_manual(dev, sampling_rate, 1);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        // ABORT(-1);
        return(result);
	}

    // Need to make more study in the future. temperily set it 0.
    result = hackrf_set_baseband_filter_bandwidth(dev, 0);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
    // ABORT(-1);
		return(result);
	}

    result = hackrf_set_vga_gain(dev, vga_gain);
	result |= hackrf_set_lna_gain(dev, lna_gain);

    if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_set_vga_gain hackrf_set_lna_gain failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
		// ABORT(-1);
		return(result);
	}

    // Center frequency
    result = hackrf_set_freq(dev, FREQ_CENTRE);
    if( result != HACKRF_SUCCESS ) {
        printf("config_hackrf hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        // ABORT(-1);
        return(result);
    }
    return(result);
}

int rx_callback(hackrf_transfer* transfer) {
	size_t bytes_to_write;
	size_t bytes_written;
	unsigned int i;
    printf("rx_callback-in\n");

	if( fd != NULL )
	{
		byte_count += transfer->valid_length;
		bytes_to_write = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_write >= bytes_to_xfer) {
				bytes_to_write = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_write;
		}

		if (stream_size>0){
		    if ((stream_size-1+stream_head-stream_tail)%stream_size <bytes_to_write) {
				stream_drop++;
		    } else {
				if(stream_tail+bytes_to_write <= stream_size) {
				    memcpy(stream_buf+stream_tail,transfer->buffer,bytes_to_write);
				} else {
				    memcpy(stream_buf+stream_tail,transfer->buffer,(stream_size-stream_tail));
				    memcpy(stream_buf,transfer->buffer+(stream_size-stream_tail),bytes_to_write-(stream_size-stream_tail));
				};
				__atomic_store_n(&stream_tail,(stream_tail+bytes_to_write)%stream_size,__ATOMIC_RELEASE);
		    }
		    return 0;
		} else {
			bytes_written = fwrite(transfer->buffer, 1, bytes_to_write, fd);
			if ((bytes_written != bytes_to_write)
				|| (limit_num_samples && (bytes_to_xfer == 0))) {
				return -1;
			} else {
				return 0;
			}
		}
	}
    else
    {
		return -1;
	}
}



int rx(hackrf_device *dev)
{
    struct timeval time_start;
    int result = 0;
    fd = fopen("./recodefiledemo.file", "wb");

    gettimeofday(&time_start, NULL);

    #ifdef DEBUG
    printf("time_start:%ld\n", 1000000*time_start.tv_sec+time_start.tv_usec);
    #endif
    result = hackrf_start_rx(dev, rx_callback, NULL);
    printf("2\n");
	// while( (hackrf_is_streaming(device) == HACKRF_TRUE) &&(do_exit == false) )
	while( (hackrf_is_streaming(dev) == HACKRF_TRUE) )
	{
        // printf("3-in\n");
        // if(byte_count > 1000000)
            // break;

        /********
         * original
         *
         * *******/

        // uint32_t byte_count_now;
		struct timeval time_now;
        gettimeofday(&time_now, NULL);


        if( (1000000*(time_now.tv_sec-time_start.tv_sec)+time_now.tv_usec - time_start.tv_usec) > 300000)
        {
            #ifdef DEBUG
            printf("time_now : %ld\n", time_now.tv_usec);
            printf("time_gap : %ld\n", 1000000*(time_now.tv_sec-time_start.tv_sec)+time_now.tv_usec - time_start.tv_usec);
            #endif
            break;
        }
		// float time_difference, rate;
		// if (stream_size>0) {
		//     if(stream_head==stream_tail) {
		// 		sleep(1); // queue empty
		//     } else {
		// 		ssize_t len;
		// 		ssize_t bytes_written;
		// 		uint32_t _st= __atomic_load_n(&stream_tail,__ATOMIC_ACQUIRE);
		// 		if(stream_head<_st)
		// 	    	len=_st-stream_head;
		// 		else
		// 	    	len=stream_size-stream_head;
		// 		bytes_written = fwrite(stream_buf+stream_head, 1, len, fd);
		// 		if (len != bytes_written) {
		// 			fprintf(stderr, "write failed");
		// 		};
		// 		stream_head=(stream_head+len)%stream_size;
		//     }
		//     if(stream_drop>0) {
		// 		uint32_t drops= __atomic_exchange_n (&stream_drop,0,__ATOMIC_SEQ_CST);
		// 		fprintf(stderr, "dropped frames: [%d]\n", drops);
		//     }
        // }
    }

	result = hackrf_stop_rx(dev);
    printf("b\n");

    if( result != HACKRF_SUCCESS ) {
		// fprintf(stderr, "hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name(result), result);
		fprintf(stderr, "hackrf_stop_rx() failed: (%d)\n", result);
	}else {
		fprintf(stderr, "hackrf_stop_rx() done\n");
	}

	// result = hackrf_close(dev);
	// if(result != HACKRF_SUCCESS) {
	// 	// fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
	// 	fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
	// } else {
	// 	fprintf(stderr, "hackrf_close() done\n");
	// }

	// // hackrf_exit();
	// fprintf(stderr, "hackrf_exit() done\n");


    return result;
}


int tx_callback(hackrf_transfer* transfer) {
	size_t bytes_to_read;
	size_t bytes_read;
	unsigned int i;

	if( fd != NULL )
	{
		byte_count += transfer->valid_length;
    // printf("transfer->valid_length is %d\n", transfer->valid_length);
		bytes_to_read = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_read >= bytes_to_xfer) {
				/*
				 * In this condition, we probably tx some of the previous
				 * buffer contents at the end.  :-(
				 */
				bytes_to_read = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_read;
		}
		bytes_read = fread(transfer->buffer, 1, bytes_to_read, fd);
		if (limit_num_samples && (bytes_to_xfer == 0)) {
                               return -1;
		}
		if (bytes_read != bytes_to_read)
        {
            return -1; /* end of file */

		} else {
			return 0;
		}
	} else {
        return -1;
    }
}

int tx(hackrf_device *dev)
{
    struct timeval time_start;
    int result = 0;
    fd = fopen("./filesource.dat", "rb");

    gettimeofday(&time_start, NULL);

    #ifdef DEBUG
    printf("time_start:%ld\n", 1000000*time_start.tv_sec+time_start.tv_usec);
    #endif
    result = hackrf_start_tx(dev, tx_callback, NULL);
    // printf("2\n");
	// while( (hackrf_is_streaming(device) == HACKRF_TRUE) &&(do_exit == false) )
	while( (hackrf_is_streaming(dev) == HACKRF_TRUE) )
	{
        // printf("3-in\n");
        // if(byte_count > 1000000)
            // break;

        /********
         * original
         *
         * *******/

        // uint32_t byte_count_now;
		struct timeval time_now;
        gettimeofday(&time_now, NULL);


        if( (1000000*(time_now.tv_sec-time_start.tv_sec)+time_now.tv_usec - time_start.tv_usec) > 3000000)
        {
            #ifdef DEBUG
            printf("time_now : %ld\n", time_now.tv_usec);
            printf("time_gap : %ld\n", 1000000*(time_now.tv_sec-time_start.tv_sec)+time_now.tv_usec - time_start.tv_usec);
            #endif
            break;
        }
		// float time_difference, rate;
		// if (stream_size>0) {
		//     if(stream_head==stream_tail) {
		// 		sleep(1); // queue empty
		//     } else {
		// 		ssize_t len;
		// 		ssize_t bytes_written;
		// 		uint32_t _st= __atomic_load_n(&stream_tail,__ATOMIC_ACQUIRE);
		// 		if(stream_head<_st)
		// 	    	len=_st-stream_head;
		// 		else
		// 	    	len=stream_size-stream_head;
		// 		bytes_written = fwrite(stream_buf+stream_head, 1, len, fd);
		// 		if (len != bytes_written) {
		// 			fprintf(stderr, "write failed");
		// 		};
		// 		stream_head=(stream_head+len)%stream_size;
		//     }
		//     if(stream_drop>0) {
		// 		uint32_t drops= __atomic_exchange_n (&stream_drop,0,__ATOMIC_SEQ_CST);
		// 		fprintf(stderr, "dropped frames: [%d]\n", drops);
		//     }
        // }
    }

	result = hackrf_stop_tx(dev);
    // printf("b\n");

    if( result != HACKRF_SUCCESS ) {
		// fprintf(stderr, "hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name(result), result);
		fprintf(stderr, "hackrf_stop_rx() failed: (%d)\n", result);
	}else {
		fprintf(stderr, "hackrf_stop_tx() done\n");
	}

	// result = hackrf_close(dev);
	// if(result != HACKRF_SUCCESS) {
	// 	// fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
	// 	fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
	// } else {
	// 	fprintf(stderr, "hackrf_close() done\n");
	// }

	// // hackrf_exit();
	// fprintf(stderr, "hackrf_exit() done\n");


    return result;
}

int main()
{
    hackrf_device *hackrf_dev = NULL;
    int result = 0;

    if (config_hackrf(hackrf_dev, 0) ==0)
    {
        cout << "HACKRF device FOUND!\n";
    }
    else
    {
        cout << "HACKRF device not FOUND!\n";
    //     ABORT(-1);
    }

    //testing
    for(int i = 0;i<10;i++)
    {
        tx(hackrf_dev);
        // rx(hackrf_dev);
    }

    result = hackrf_close(hackrf_dev);
	if(result != HACKRF_SUCCESS) {
		// fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
	} else {
		fprintf(stderr, "hackrf_close() done\n");
	}

	hackrf_exit();
	fprintf(stderr, "hackrf_exit() done\n");

}
