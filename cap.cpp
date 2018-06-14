
#include "cap.h"
#include "common.h"

#include <hackrf.h>
using namespace std;


int8 hackrf_rx_buf[CAPLENGTH*2];  // used for capture_data() and hackrf rx callback
int hackrf_rx_count;  // used for capture_data() and hackrf rx callback

static int capbuf_hackrf_callback(hackrf_transfer* transfer) {
  size_t bytes_to_write;
  size_t hackrf_rx_count_new = hackrf_rx_count + transfer->valid_length;

  int count_left = (CAPLENGTH*2) - hackrf_rx_count_new;
  if ( count_left <= 0 ) {
    bytes_to_write = transfer->valid_length + count_left;
  } else {
    bytes_to_write = transfer->valid_length;
  }

//  cout << transfer->valid_length  << " " << hackrf_rx_count << " " << bytes_to_write << "\n";
  if (bytes_to_write!=0)
  {
    memcpy( hackrf_rx_buf+hackrf_rx_count, transfer->buffer, bytes_to_write );
//    for (size_t i=0; i<bytes_to_write; i++) {
//      hackrf_rx_buf[hackrf_rx_count+i] = transfer->buffer[i];
//    }
    hackrf_rx_count = hackrf_rx_count + bytes_to_write;
  }
//  cout << transfer->valid_length  << " " << hackrf_rx_count << " " << bytes_to_write << "\n";

  return(0);
}


// This function produces a vector of captured data. The data can either
// come from live data received by the RTLSDR, or from a file containing
// previously captured data.
// Also, optionally, this function can save each set of captured data
// to a file.
int capture_data(
  // Inputs
  const double & fc_requested,
  const bool & save_cap,    //save the captured file 
  const char * record_bin_filename,
  const string & data_dir,
  hackrf_device * & hackrf_dev,
  // Output
  cvec & capbuf,
  double & fc_programmed,
  double & fs_programmed,
) {
  // Filename used for recording or loading captured data.
  static uint32 capture_number=0;
  stringstream filename;
  filename << data_dir << "/capbuf_" << setw(4) << setfill('0') << capture_number << ".it";

//  cout << use_recorded_data << "\n";
//  cout << load_bin_filename << "\n";

  bool record_bin_flag = (strlen(record_bin_filename)>4);


  int run_out_of_data = 0;

    if (verbosity>=2) {
      cout << "Capturing live data" << endl;
    }
      
    if (1) {
      fc_programmed = fc_requested;

      // Center frequency
      int result = hackrf_set_freq(hackrf_dev, fc_requested);
      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      result = hackrf_stop_rx(hackrf_dev);
      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      hackrf_rx_count = 0; // clear counter
      result = hackrf_start_rx(hackrf_dev, capbuf_hackrf_callback, NULL);

      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_start_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      while(hackrf_is_streaming(hackrf_dev) == HACKRF_TRUE) {
//        cout << hackrf_rx_count << "\n";
        if( hackrf_rx_count == (CAPLENGTH*2) )
          break;
      }

      result = hackrf_is_streaming(hackrf_dev);

//      result = hackrf_stop_rx(hackrf_dev);
//      if( result != HACKRF_SUCCESS ) {
//        printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//        ABORT(-1);
//      }

      // Convert to complex
      capbuf.set_size(CAPLENGTH, false);
      for (uint32 t=0;t<CAPLENGTH;t++) {
//        capbuf(t)=complex<double>((((double)hackrf_rx_buf[(t<<1)])-128.0)/128.0,(((double)hackrf_rx_buf[(t<<1)+1])-128.0)/128.0);
        capbuf(t)=complex<double>((((double)hackrf_rx_buf[(t<<1)])-0.0)/128.0,(((double)hackrf_rx_buf[(t<<1)+1])-0.0)/128.0);
      }

    } 
  

  // Save the capture data, if requested.
  if (save_cap) {
    if (verbosity>=2) {
      cout << "Saving captured data to file: " << filename.str() << endl;
    }
    it_file itf(filename.str(),true);
    itf << Name("capbuf") << capbuf;

    ivec fc_v(1);
    fc_v(0)=fc_requested;
    itf << Name("fc") << fc_v;

    ivec fc_p(1);
    fc_p(0)=fc_programmed;
    itf << Name("fcp") << fc_p;

    ivec fs_p(1);
    fs_p(0)=fs_programmed;
    itf << Name("fsp") << fs_p;

    itf.close();
  }

  if (record_bin_flag) {
    if (verbosity>=2) {
      cout << "Saving captured data to file: " << record_bin_filename << endl;
    }
    FILE *fp = NULL;
    if (capture_number==0){
      fp = fopen(record_bin_filename, "wb");
    } else {
      fp = fopen(record_bin_filename, "ab");
    }

    if (fp == NULL)
    {
      cerr << "capture_data Error: unable to open file: " << record_bin_filename << endl;
      ABORT(-1);
    }

    if (capture_number==0){ // write bin file header
      int ret = write_header_to_bin(fp, fc_requested,fc_programmed,(const double &)1920000,fs_programmed); // not use fs. it seems always 1920000
      if (ret) {
        cerr << "capture_data Error: unable write header info to file: " << record_bin_filename << endl;
        ABORT(-1);
      }
    }

    for (uint32 t=0;t<CAPLENGTH;t++) {
      unsigned char tmp;
      tmp = (unsigned char)( capbuf(t).real()*128.0 + 128.0 );
      fwrite(&tmp, sizeof(unsigned char), 1, fp);
      tmp = (unsigned char)( capbuf(t).imag()*128.0 + 128.0 );
      fwrite(&tmp, sizeof(unsigned char), 1, fp);
    }
    fclose(fp);
  }

  capture_number++;
  return(run_out_of_data);
}

