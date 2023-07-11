#ifndef FFT_H_
#define FFT_H_

#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

enum fft_isign {
  FFT_FORWARD = 1,
  FFT_INVERSE = -1
};

// Data is a 2*nn array where data[0] + i data[1] is the first complex
// given to the FFT.
// isign = 1  -> forward fourier transform
// isign = -1 -> inverse fourier transform
// For inverse fourier, coefficients should be multiplied by 2/n.
void SYMPH_API dfft(float data[], unsigned int nn, enum fft_isign isign);

// Compute the FFT from a real array of size nn.
// The output is the first half of the fourier transform
// stored into data.
// The first complex outputed by the algorithm is data[2] + i data[3].
// Cooridnates 0 and 1 are respectively the 0th real and nn'th real value.
// For 0 and nn the imaginary part is always 0.
// For inverse fourier, coefficients should be multiplied by 2/n.
void SYMPH_API realft(float data[], unsigned int nn, enum fft_isign isign);

// Fill window with a hann window
void SYMPH_API compute_hann(float* window, unsigned int window_size);
// Fill window with a hamming window
void SYMPH_API compute_hamming(float* window, unsigned int window_size);
// Fill window with a blackman-harris window
void SYMPH_API compute_blackman_harris(float* window, unsigned int window_size);

#ifdef __cplusplus
}
#endif

#endif /* !FFT_H_ */
