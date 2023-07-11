#ifndef V2P_MATH_H_
#define V2P_MATH_H_

#ifdef __cplusplus
extern "C" {
#endif

// Require standard C math header
#include <math.h>

#ifndef M_PI // Yes, in 2019 M_PI is still not standard...
  #define M_PI 3.14159265358979323846
#endif

#define _max(x,y) ((x) >= (y)) ? (x) : (y)
#define _min(x,y) ((x) <= (y)) ? (x) : (y)

static float fabs_max_arr(float* array, unsigned int length) {
  float max = 0;
  for (unsigned int i = 0; i < length; i++) {
    const float v = (float)fabs(array[i]);
    if (v > max)
      max = v;
  }
  return max;
}

static unsigned int fabs_argmax(float* array, unsigned int length) {
  float max = 0;
  unsigned int idx = 0;
  for (unsigned int i = 0; i < length; i++) {
    const float v = (float)fabs(array[i]);
    if (v > max) {
      max = v;
      idx = i;
    }
  }
  return idx;
}

// Gets the minimum and maximum values of an array
static void __get_min_max(float* window, unsigned int window_length, float *min, float *max)
{
  *min = window[0];
  *max = window[0];
  for (unsigned int i = 0; i < window_length; i++) {
    if (window[i] < *min)
      *min = window[i];
    if (window[i] > *max)
      *max = window[i];
  }
}


//! Callback used to sort an array of candidates by amplitude in dsc order.
int dsc_candidates_amplitude(const void* elem1, const void* elem2);

//! Macro which allow to easily access
#define symp_path_at(s, y, x) ((s)->nb_candidates_per_step * (y) + (x))

//! Allow swapping two variables of any type
#define symp_swap(x,y) do \
   { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (signed)sizeof(x) : -1]; \
     memcpy(swap_temp,&y,sizeof(x)); \
     memcpy(&y,&x,       sizeof(x)); \
     memcpy(&x,swap_temp,sizeof(x)); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* !V2P_MATH_H_ */
