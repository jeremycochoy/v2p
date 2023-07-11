#include "autocorrelation.h"
#include "fft.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

unsigned int SYMPH_API next_power_of_two(unsigned int v)
{
  v -= 1;
  v = v | (v >> 1);
  v = v | (v >> 2);
  v = v | (v >> 4);
  v = v | (v >> 8);
  v = v | (v >> 16);
  return v + 1;
}

static inline float* __compute_autocorrelation(float* frame_in, unsigned int size_in,
  unsigned int* size_out, bool normalize, float** remember_fft) {
  // Append a frame of zero to have a correlation function
  // define at least as long as the input frame (see behavior of fft).
  unsigned int ac_length = size_in * 2;
  ac_length = next_power_of_two(ac_length);
  float* frame_out = malloc(sizeof(*frame_out) * ac_length);
  if (!frame_out)
    return frame_out;

  memcpy(frame_out, frame_in, sizeof(*frame_in) * size_in);
  memset(frame_out + size_in, 0, sizeof(*frame_in) * (ac_length - size_in));

  // Compute the fourier transform and powerDensity
  realft(frame_out, ac_length, FFT_FORWARD);

  // Make a copy of the fft in case other algorithms require to analyze it
  if (remember_fft) {
    (*remember_fft) = malloc(sizeof(**remember_fft) * ac_length);
    if (*remember_fft)
        memcpy(*remember_fft, frame_out, sizeof(*frame_out) * ac_length);
  }

  // Compute the power density (squared norm of complex values)
  frame_out[0] *= frame_out[0];
  frame_out[1] *= frame_out[1];
  for (size_t i = 2; i < ac_length; i += 2) {
    const float x = frame_out[i];
    const float y = frame_out[i + 1];
    frame_out[i] = x * x + y * y;
    frame_out[i + 1] = 0;
  }

  // We compute the inverse fourier
  // Notice that the second half of frame_out
  // is the reverse of the first half, and therefore not required.
  realft(frame_out, ac_length, FFT_INVERSE);

  // Normalizing can be unactivated for better performances
  if (normalize) {
    const float normalizer = 2.0f / ac_length;
    for (unsigned int i = 0; i < ac_length; i++)
      frame_out[i] *= normalizer;
  }

  // Return autocorrelation
  // Only the first path of the function returned by IFFT is relevent.
  // Indeed, the computed function ac(x) is
  // symetrical: ac(x) = ac(ac_length - x).
  *size_out = ac_length / 2;
  return frame_out;
}

float* compute_autocorrelation(float* frame_in, unsigned int size_in,
  unsigned int* size_out) {
  return __compute_autocorrelation(frame_in, size_in, size_out, true, NULL);
}

float* compute_autocorrelation_and_fft(float* frame_in, unsigned int size_in,
  unsigned int* size_out, float** fft) {
  return __compute_autocorrelation(frame_in, size_in, size_out, true, fft);
}

float* compute_unnormalized_autocorrelation(float* frame_in, unsigned int size_in,
  unsigned int* size_out) {
  return __compute_autocorrelation(frame_in, size_in, size_out, false, NULL);
}

float* compute_unnormalized_autocorrelation_and_fft(float* frame_in, unsigned int size_in,
  unsigned int* size_out, float** fft) {
  return __compute_autocorrelation(frame_in, size_in, size_out, false, fft);
}

static inline
float SYMPH_API* __compute_corrected_autocorrelation(
  float* frame_in,
  float* window_in,
  float** window_ac /* = NULL */,
  float** fft /* = NULL */,
  unsigned int size_in,
  unsigned int* size_out) {
  // Compute average
  float average = 0;
  for (unsigned int i = 0; i < size_in; i++)
    average += frame_in[i];
  average /= size_in;

  // Copy the input frame
  const size_t chunk_size = sizeof(*frame_in) * size_in;
  float* frame_in_cpy = malloc(chunk_size);
  memcpy(frame_in_cpy, frame_in, chunk_size);

  // Apply window
  for (unsigned int i = 0; i < size_in; i++) {
    const float v = frame_in_cpy[i];
    frame_in_cpy[i] = (v - average) * window_in[i];
  }

  // Compute corrected autocorrelation of the frame
  float* autocorrelation = compute_unnormalized_autocorrelation_and_fft(
    frame_in_cpy, size_in, size_out, fft
  );
  float* window_ac_ptr;
  // In case we already have the autocorrelation, retrieve it
  if (window_ac && *window_ac)
    window_ac_ptr = *window_ac;
  // otherwise compute it.
  else
    window_ac_ptr = compute_unnormalized_autocorrelation(
      window_in, size_in, size_out
    );

  // Corrected autocorrelation isn't correct after 1/2
  // of the window. Therefore we keep only half of it.
  *size_out = (*size_out) / 2;

  // Correct autocorrelation
  for (unsigned int i = 0; i < *size_out; i++)
    autocorrelation[i] = autocorrelation[i] / window_ac_ptr[i];

  // Free frame_in copy
  free(frame_in_cpy);
  // Free window_ac_ptr or store it in case we computed it
  if (!window_ac)
    free(window_ac_ptr);
  else if (window_ac && !*window_ac)
    *window_ac = window_ac_ptr;

  return autocorrelation;
}

float SYMPH_API* compute_corrected_autocorrelation(
  float* frame_in,
  float* window_in,
  float** window_ac /* = NULL */,
  unsigned int size_in,
  unsigned int* size_out)
{
  return __compute_corrected_autocorrelation(
    frame_in, window_in, window_ac, NULL, size_in, size_out
  );
}

float SYMPH_API* compute_corrected_autocorrelation_and_fft(
  float* frame_in,
  float* window_in,
  float** window_ac /* = NULL */,
  float** fft /* = NULL */,
  unsigned int size_in,
  unsigned int* size_out)
{
  return __compute_corrected_autocorrelation(
    frame_in, window_in, window_ac, fft, size_in, size_out
  );
}