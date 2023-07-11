#ifndef AUTOCORRELATION_H_
#define AUTOCORRELATION_H_

#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Compute autocorrelation of the frame_in window of size size_in.
//! @return allocated string of size size_out.
float SYMPH_API* compute_autocorrelation(float* frame_in, unsigned int size_in, unsigned int* size_out);

//! Same as compute_autocorrelation but faster because of the abscense of normalization
float SYMPH_API* compute_unnormalized_autocorrelation(float* frame_in, unsigned int size_in, unsigned int* size_out);

//! Compute autocorrelation of frame_in with the window window_in.
//! Both argument should have a size of size_in.
//!
//! @param frame_in Input data.
//! @param window_in Window to apply before computing autocorrelation.
//! @argument window_ac Optional reference to autocorrelation of the window.
//!                     If not given it is allocated and computed by
//!                     this function at each call.
//!                     If a reference to a float* is given, it will allocate
//!                     the value at the first run and reuse it later.
//! @param size_in The length of both frame_in and window_in.
//! @param[out] size_out The size of the autocorrelation computed.
//! The returned pointer should be free.
float SYMPH_API* compute_corrected_autocorrelation(
  float* frame_in,
  float* window_in,
  float** window_ac /* = NULL */,
  unsigned int size_in,
  unsigned int* size_out);

//! Same as compute_corrected_autocorrelation but return a mallocated fft
float SYMPH_API* compute_corrected_autocorrelation_and_fft(
  float* frame_in,
  float* window_in,
  float** window_ac /* = NULL */,
  float** fft /* = NULL */,
  unsigned int size_in,
  unsigned int* size_out);

//! Compute the next power of 2 of a positive integer
//! @param v Positive integer above or equal to 2
unsigned int SYMPH_API next_power_of_two(unsigned int v);

#ifdef __cplusplus
}
#endif

#endif /* !AUTOCORRELATION_H_ */
