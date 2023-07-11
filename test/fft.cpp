#include "lib/TestHarness.hpp"
#include "fft.h"

#include <vector>
#include <algorithm>

TEST (FFT, FFT_of_zero_vector_is_zero)
{
  // FFT of 0 should be 0
  for (int size = 4; size <= 64; size *= 2) {
    std::vector<float> data(size * 2, 0);

    dfft(&data[0], size, FFT_FORWARD);

    for (int i = 0; i < size * 2; i++)
      CHECK_DOUBLES_EQUAL(0, data[i]);
  }
}

TEST (FFT, IFFT_composed_with_FFT_is_identity)
{
  int size = 1024;
  std::vector<float> data(size * 2);

  // Generate random complex numbers
  std::generate(data.begin(), data.end(),
    []() { return (float)rand()/(float)(RAND_MAX / 1000.0f) - 5.0f; }
  );

  // Make a copy of the original data
  auto original_data(data);

  dfft(&data[0], size, FFT_INVERSE);
  dfft(&data[0], size, FFT_FORWARD);

  for (int i = 0; i < size * 2; i++)
    CHECK_DOUBLES_EQUAL(original_data[i], data[i] * 1 / size);
}

TEST (FFT, REALFT_of_zero_vector_is_0)
{
  std::vector<float> data(1024);

  realft(&data[0], data.size(), FFT_FORWARD);

  for (const auto& dt : data)
    CHECK_DOUBLES_EQUAL(0, dt);
}


TEST (FFT, IREALFT_composed_with_REALFT_is_identity)
{
  std::vector<float> data(1024);

  // Generate random complex numbers
  std::generate(data.begin(), data.end(),
    []() { return (float)rand()/(float)(RAND_MAX / 1000.0f) - 5.0f; }
  );

  auto original_data(data);

  realft(&data[0], data.size(), FFT_FORWARD);
  realft(&data[0], data.size(), FFT_INVERSE);

  for (unsigned int i = 0; i < data.size(); i++)
    CHECK_DOUBLES_EQUAL(original_data[i], data[i] * 2 / data.size());
}

TEST (FFT, Hann_Window)
{
  std::vector<float> window((rand() % 1234) + 1, 0);

  compute_hann(&window[0], window.size());

  // Hann window is positive
  for (const auto& elm : window)
    CHECK(elm >= 0);

  // Maximum of a window is 1
  const auto max = std::max_element(window.begin(), window.end());
  CHECK_DOUBLES_EQUAL(1, *max);

  // Hann window is 0 at extremities
  CHECK_DOUBLES_EQUAL(0, window[0]);
  CHECK_DOUBLES_EQUAL(0, window[window.size() - 1]);
}

TEST (FFT, Hamming_Window)
{
  std::vector<float> window((rand() % 1234) + 1, 0);

  compute_hamming(&window[0], window.size());

  // Hamming window is positive
  for (const auto& elm : window)
    CHECK(elm >= 0);

  // Maximum of a window is 1
  const auto max = std::max_element(window.begin(), window.end());
  CHECK_DOUBLES_EQUAL(1, *max);

  // Hann window is strictly positive at extremities !
  CHECK(0.07 < window[0]);
  CHECK(0.08 > window[0]);
  CHECK(0.07 < window[window.size() - 1]);
  CHECK(0.08 > window[window.size() - 1]);
}

TEST (FFT, BlackmanHarris_Window)
{
  std::vector<float> window((rand() % 1234) + 1, 0);

  compute_blackman_harris(&window[0], window.size());

  // BH window is positive
  for (const auto& elm : window)
    CHECK(elm >= 0);

  // Maximum of a window is 1
  const auto max = std::max_element(window.begin(), window.end());
  CHECK_DOUBLES_EQUAL(1, *max);

  // BH window is 0 at extremities
  CHECK_DOUBLES_EQUAL(0, window[0]);
  CHECK_DOUBLES_EQUAL(0, window[window.size() - 1]);
}
