#ifndef CAP_H
#define CAP_H
#define CAPLENGTH 153600


int capture_data(
  // Inputs
  const double & fc_requested,
  const bool & save_cap,    //save the captured file 
  const char * record_bin_filename,
  const std::string & data_dir,
  hackrf_device * & hackrf_dev,
  // Output
  cvec & capbuf,
  double & fc_programmed,
  double & fs_programmed,
);

#endif