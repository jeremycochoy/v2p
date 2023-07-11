#include "fft.h"
#include "tools.h"
#include "v2p.h"
#include "stdlib.h"
#include "stdbool.h"

#define SWAP(a,b) tempr=(a); (a)=(b); (b)=tempr

// Hash table used to precompute cosinus and sinus tables
#define SINCOS_TAB_SIZE 256
static double sin_table[SINCOS_TAB_SIZE];
static double cos_table[SINCOS_TAB_SIZE];

uint log2ui(uint n) {
  uint log = 0;
  while(n >>= 1) log++;
  return log;
}

void __precompute_sincos() {
  const double theta = 2 * M_PI;
  for (uint i = 0; i < SINCOS_TAB_SIZE; i++) {
    sin_table[i] = sin(theta / pow(2., i));
    cos_table[i] = cos(theta / pow(2., i));
  }
}

static inline
void initialize_sincos_tables() {
  static bool initialized = false;
  if (!initialized) {
    __precompute_sincos();
    initialized = true;
  }
}


//
// See the book:
// Numerical Recipes in C | The Art of Scientific Computing
// Beware of the indices.
//

// Replaces data[0..2*nn-1] by its discrete Fourier transform,
// if isign is input as 1; or replaces data[0..2*nn-1] by nn times its inverse
// discrete Fourier transform, if isign is input as −1.
// data is a complex array of length nn or, equivalently, a real array
// of length 2*nn.
// nn MUST be an integer power of 2 (this is not checked for!).
void dfft(float data[], unsigned int nn, int isign) {
  initialize_sincos_tables();
  unsigned int n, mmax, m, j, istep, i;
  double wtemp, wr, wpr, wpi, wi;
  float tempr, tempi;
  n = nn << 1;
  j = 1;

  // This is the bit-reversal section of the routine.
  for (i = 1; i < n; i += 2) {
    if (j > i) {
      SWAP(data[j-1], data[i-1]);
      // Exchange the two complex numbers.
      SWAP(data[j], data[i]);
    }
    m = nn;
    while (m >= 2 && j > m) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }
  // Here  begins  the  Danielson-Lanczos  section  of  the  routine.
  mmax = 2;
  uint theta_index = 1;
  while (n > mmax) {
    // Outer loop executed log2(nn) times.
    istep = mmax << 1;
    // Initialize the trigonometric recurrence.
    //The angle considered is theta = isign * (6.28318530717959 / mmax)
    wtemp = sin_table[theta_index + 1]; // abs(sin(theta / 2))
    wpr = -2.0 * wtemp * wtemp;
    wpi = isign * sin_table[theta_index]; // sin(theta)
    wr = 1.0;
    wi = 0.0;
    for (m = 1; m < mmax; m += 2) { // Here are the two nested inner loops.
      for (i = m; i <= n; i += istep) {
        j = i + mmax;
        // This  is  the  Danielson-Lanczos  formula:
        tempr = (float)(wr * data[j - 1] - wi * data[j]);
        tempi = (float)(wr * data[j] + wi * data[j - 1]);
        data[j - 1] = data[i - 1] - tempr;
        data[j] = data[i] - tempi;
        data[i - 1] += tempr;
        data[i] += tempi;
      }
      wtemp = wr;
      wr += wr * wpr - wi * wpi;
      //Trigonometric  recurrence.
      wi += wi * wpr + wtemp * wpi;
    }
    mmax = istep;
    theta_index++;
  }
}

// Calculates the Fourier transform of a set of n real-valued data points.
// Replaces this data (whichis stored in array data[0..n-1]) by the positive
// frequency half of its complex Fourier transform.
// The real-valued first and last components of the complex transform are
// returned as elements data[0] and data[1], respectively.
// n must be a power of 2.
// This routine also calculates theinverse transform of a complex data array
// if it is the transform of real data.
// (Result in this case must be multiplied by 2/n.)
//
// The output looks like:
// data[0] --- real component of transform point 0
// data[1] --- real component of transform point 512
// data[2] --- real component of transform point 1
// data[3] --- imaginary component of transform point 1
// data[4] --- real component of transform point 2
// .
// data[1022] --- real component of transform point 511
// data[1023] --- imaginary component of transform point 511
//
void realft(float data[], unsigned int n, int isign) {
  initialize_sincos_tables();
  unsigned int i, i1, i2, i3, i4, np1;
  float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
  double wr, wi, wpr, wpi, wtemp;
  // Double precision for the trigonometric recurrences.
  double theta = M_PI / (double)(n >> 1);
  uint theta_index = log2ui(n);
  // Initialize the recurrence.
  if (isign == 1) {
    // The forward transform is here.
    c2 = -0.5;
    dfft(data, n >> 1, 1);
  } else {
    // Otherwise set up for an inverse trans-form.
    c2 = 0.5;
    theta = -theta;
    isign = -1;
  }
  wtemp = isign * sin_table[theta_index + 1];
  wpr = -2.0 * wtemp * wtemp;
  wpi = isign * sin_table[theta_index];
  wr = 1.0 + wpr;
  wi = wpi;
  np1 = n + 1;
  for (i = 1; i < (n >> 2); i++) {
    // Case i=1 done separately below.
    i1 = i + i;
    i2 = 1 + i1;
    i3 = np1 - i2;
    i4 = 1 + i3;
    //The two separate transforms are separated out of data.
    h1r = c1 * (data[i1] + data[i3]);
    h1i = c1 * (data[i2] - data[i4]);
    h2r = -c2 * (data[i2] + data[i4]);
    h2i = c2 * (data[i1] - data[i3]);
    // Here they are recombined to form the true transform of the origi-nalrealdata.
    data[i1] = (float)(h1r + wr * h2r - wi * h2i);
    data[i2] = (float)(h1i + wr * h2i + wi * h2r);
    data[i3] = (float)(h1r - wr * h2r + wi * h2i);
    data[i4] = (float)(-h1i + wr * h2i + wi * h2r);
    wr = (wtemp = wr) * wpr - wi * wpi + wr;
    // The recurrence.
    wi = wi * wpr + wtemp * wpi + wi;
  }
  if (isign == 1) {
    // Squeeze the first and last data to-gether to get them all within the
    // original array.
    data[0] = (h1r = data[0]) + data[1];
    data[1] = h1r - data[1];
  } else {
    data[0] = c1 * ((h1r = data[0]) + data[1]);
    data[1] = c1 * (h1r - data[1]);
    // This is the inverse transform for the case isign = -1.
    dfft(data, n >> 1, -1);
  }
}

void compute_hann(float* window, unsigned int window_size) {
  for (unsigned int i = 0; i < window_size; i++) {
    double v = sin(M_PI * i / (window_size-1));
    window[i] = (float)(v * v);
  }
}

void compute_hamming(float* window, unsigned int window_size) {
    double a0 = 0.53836;
    double a1 = 0.46164;
    for (unsigned int i = 0; i < window_size; i++) {
      window[i] = (float)(
        a0 - a1 * cos(2.f * M_PI * i / (window_size-1))
      );
    }
}

void compute_blackman_harris(float* window, unsigned int window_size) {
  // List of all coefficients:
  // https://sci-hub.tw/10.1109/icassp.2001.940309
  const unsigned int N = window_size;
  for (unsigned int n = 0; n < window_size; n++) {
    double value = 0.f;
    double list[] = {
        2.384331152777942e-001,
        4.005545348643820e-001,
        2.358242530472107e-001,
        9.527918858383112e-002,
        2.537395516617152e-002,
        4.152432907505835e-003,
        3.685604163298180e-004,
        1.384355593917030e-005,
        1.161808358932861e-007
      };
    for (unsigned int index = 0; index < sizeof(list) / sizeof(list[0]); index++) {
      const double an = list[index];
      const double sign = pow(-1.0, index);
      value += sign * an * cos(2 * index * M_PI * n / (N - 1));
    }
    window[n] = (float)value;
  }
}
