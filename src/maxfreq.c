#include "v2p.h"
#include "tools.h"
#include "maxfreq.h"
#include "boersma.h"
#include "fft.h"

#include <stdlib.h>
#include <string.h>

#define log2(x) (log(x) / log(2))

algorithm_descriptor_maxfreq_t* maxfreq_init(
    algorithm_descriptor_maxfreq_t* ad, unsigned int frame_size) {
    memset(ad, 0, sizeof(*ad));

    //Default parameters
    if (frame_size == 0)
      frame_size = 2048;

    ad->parent.frame_size = frame_size;
    ad->parent.nb_candidates_per_step = 1;
    ad->parent.generate_candidates = (algorithm_t)generate_maxfreq_candidates;
    ad->parent.generate_frame = (framer_t)generate_frame_boersma;

    return ad;
}

algorithm_descriptor_maxfreq_t* maxfreq_new(uint frame_size) {
    void* ptr = calloc(1, sizeof(struct algorithm_descriptor_maxfreq));

    if(!ptr)
      return NULL;

    return maxfreq_init(ptr, frame_size);
}

void maxfreq_delete(algorithm_descriptor_maxfreq_t* ad) {
    free(ad);
}

//! Compute argument of maximal norm
static unsigned int fft_argmax_max_sum(float* array, unsigned int length, float* out_max, float* out_sum) {
  float max = log2(1 + fabs(array[0])); // real value
  float sum = log2(1 + fabs(array[0]));
  unsigned int idx = 0;
  unsigned int i;
  for (i = 1; i < length / 2 - 1; i++) {
    const float x = array[2*i];
    const float y = array[2*i + 1];
    const float v = log2(1 + x*x + y*y);
    sum += v;
    if (v > max) {
      max = v;
      idx = i;
    }
  }
  const float v = log2(1 + fabs(array[i]));
  sum += v;
  if (max < v) {
    max = v;
    idx = i;
  }
  *out_max = max;
  *out_sum = sum;
  return idx;
}

void __maxfreq_compute_weigth(struct pitch_analyzer* s,
  candidate_t* candidate) {
  const float t_max = 1.f / candidate->frequency;
  const float r_max = candidate->amplitude;
  const float log_coef = (float)log2(s->minimal_frequency * t_max);
  candidate->weight = r_max - s->octave_cost * log_coef;
}

candidate_t* generate_maxfreq_candidates(
  pitch_analyzer_t* s,
  algorithm_descriptor_maxfreq_t* ad,
  float* frame_in) {
  // Alloc a new candidates (only one is used. others are just zeros)
  candidate_t* candidates = calloc(ad->parent.nb_candidates_per_step, sizeof(*candidates));

  // Get index of maximal value
  // Remember coord 0 and 1 are real value of first and last real coefficients
  float max, sum;
  uint argmax = fft_argmax_max_sum(s->last_fft, s->last_fft_size, &max, &sum);
  float mean = sum / (s->last_fft_size / 2);
  // Convert index to frequency (see http://wiki.analytica.com/index.php?title=FFT)
  const float delta_t = 1.f / s->sampling_rate;
  const float delta_f = 1.f / (delta_t * s->last_fft_size);
  candidates->frequency = argmax * delta_f;
  candidates->amplitude = 1.f - (mean / max); // max/max - mean/max
  // The variation of amplitudes are skewed beetween 0.95 and 0.999, so we skew it
  // with a power... (Pure WTF Heuristic)
  candidates->amplitude = pow(candidates->amplitude, 2);
  __maxfreq_compute_weigth(s, candidates);

  if (candidates->frequency > 2500 || candidates->frequency < 880) {
    candidates->frequency = 0;
    candidates->weight = 0;
   }

  return candidates;
}
