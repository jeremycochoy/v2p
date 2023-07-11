#include "lib/TestHarness.hpp"
#include "autocorrelation.h"
#include "v2p.h"
#include "fft.h"

#include <vector>
#include <algorithm>
#include <cstdlib>

TEST (FFT, Next_power_of_two)
{
  CHECK_LONGS_EQUAL(32, next_power_of_two(31));
  CHECK_LONGS_EQUAL(32, next_power_of_two(27));
  CHECK_LONGS_EQUAL(16, next_power_of_two(12));
  CHECK_LONGS_EQUAL(512, next_power_of_two(300));

  for (unsigned int i = 4; i <= 4096; i*= 2) {
    CHECK_LONGS_EQUAL(i, next_power_of_two(i - 1));
    CHECK_LONGS_EQUAL(i * 2, next_power_of_two(i + 1));
  }
}

TEST (FFT, Autocorrelation_of_zero_is_zero)
{
  for (unsigned int size = 64; size <= 1024; size *= 2) {
    std::vector<float> data(size, 0);

    unsigned int new_size;
    float* out = compute_autocorrelation(&data[0], size, &new_size);

    // Autocorrelation frame is at least 1/2 of first frame
    CHECK(new_size >= size / 2);

    // autocorrelation of null function should be null
    for (unsigned int i = 0; i < new_size; i++)
      CHECK_DOUBLES_EQUAL(0, out[i]);

    // Free allocated memory
    v2p_ptr_free(out);
  }
}

TEST (FFT, Autocorrelation_of_dirac_is_dirac)
{
  unsigned int size = 42 + (rand() * 1000)/RAND_MAX;
  std::vector<float> data(size, 0);
  data[0] = 1.0f;

  unsigned int new_size;
  float* out = compute_autocorrelation(&data[0], size, &new_size);

  // autocorrelation of dirac should be a dirac
  for (unsigned int i = 1; i < new_size; i++)
    CHECK_DOUBLES_EQUAL(0, out[i]);
  CHECK_DOUBLES_EQUAL(1.0f, out[0]);

  // Free allocated memory
  v2p_ptr_free(out);
}

TEST (FFT, Corrected_autocorrelation_of_zero_is_zero)
{
  // Create data
  unsigned int size = 64;
  std::vector<float> data(size, 0);

  // Create window
  std::vector<float> window(size, 0);
  compute_hann(&window[0], size);

  // Compute AC
  unsigned int new_size;
  float* out = compute_corrected_autocorrelation(&data[0], &window[0], NULL, size, &new_size);

  // autocorrelation of 0 should be 0
  for (unsigned int i = 0; i < new_size; i++)
    CHECK_DOUBLES_EQUAL(0, out[i]);

  // Free allocated memory
  v2p_ptr_free(out);
}

TEST (FFT, Corrected_autocorrelation_of_dirac_is_dirac)
{
  // Create data
  unsigned int size = 1024;
  std::vector<float> data(size, 0);
  data[0] = 1.0f;

  // Rectangle window
  std::vector<float> window(size, 1.0);

  // Compute AC
  unsigned int new_size;
  float* out = compute_corrected_autocorrelation(&data[0], &window[0], NULL, size, &new_size);

  // autocorrelation values check
  for (unsigned int i = 1; i < new_size; i++)
    CHECK_DOUBLES_EQUAL(0, out[i]);
  CHECK(out[0] > 0);

  // Free allocated memory
  v2p_ptr_free(out);
}
