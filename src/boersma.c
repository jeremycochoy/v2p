#include "v2p.h"
#include "tools.h"
#include "autocorrelation.h"
#include "fft.h"
#include "boersma.h"
#include "stretchy_buffer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define log2(x) (log(x) / log(2))


algorithm_descriptor_boersma_t* boersma_init(
  algorithm_descriptor_boersma_t* ad, uint frame_size, uint nb_candidates) {
  memset(ad, 0, sizeof(*ad));

  // Default parameters
  if (nb_candidates == 0)
    nb_candidates = 3;
  if (frame_size == 0)
    frame_size = 2048;

  ad->parent.frame_size = frame_size;
  ad->parent.nb_candidates_per_step = nb_candidates;
  ad->parent.generate_candidates = (algorithm_t)generate_boersma_candidates;
  ad->parent.generate_frame = (framer_t)generate_frame_boersma;

  ad->voiced_window = 0;
  ad->voiced_window_ac = 0;
  ad->hnr_window = 0;
  ad->hnr_window_ac = 0;

  ad->voiced_window = malloc(sizeof(*ad->voiced_window) * ad->parent.frame_size);
  compute_hann(ad->voiced_window, ad->parent.frame_size);

  return ad;
}

void boersma_delete(algorithm_descriptor_boersma_t* b) {
    free(b->voiced_window);
    if(b->voiced_window_ac)
      b->voiced_window_ac = (free(b->voiced_window_ac), NULL);
    free(b);
}

void boersma_unvoiced_delete(algorithm_descriptor_boersma_unvoiced_t* ad) {
    free(ad);
}

algorithm_descriptor_boersma_unvoiced_t* boersma_unvoiced_init(
    algorithm_descriptor_boersma_unvoiced_t* ad, unsigned int frame_size) {
    memset(ad, 0, sizeof(*ad));

    //Default parameters
    if (frame_size == 0)
      frame_size = 2048;

    ad->parent.frame_size = frame_size;
    ad->parent.nb_candidates_per_step = 1;
    ad->parent.generate_candidates = (algorithm_t)generate_boersma_unvoiced_candidates;
    ad->parent.generate_frame = (framer_t)generate_frame_boersma;

    return ad;
}

algorithm_descriptor_boersma_t* boersma_new(uint frame_size, uint nb_candidates) {
    void* ptr = calloc(1, sizeof(struct algorithm_descriptor_boersma));

    if(!ptr)
      return NULL;

    return boersma_init(ptr, frame_size, nb_candidates);
}

algorithm_descriptor_boersma_unvoiced_t* boersma_unvoiced_new(uint frame_size) {
    void* ptr = calloc(1, sizeof(struct algorithm_descriptor_boersma_unvoiced));

    if(!ptr)
      return NULL;

    return boersma_unvoiced_init(ptr, frame_size);
}

int dsc_candidates_amplitude(const void* elem1, const void* elem2)
{
    const candidate_t a = *((const candidate_t*)elem1);
    const candidate_t b = *((const candidate_t*)elem2);
    if (a.amplitude < b.amplitude) return  1;
    if (a.amplitude > b.amplitude) return -1;
    return 0;
}

void __boersma_compute_weigth(struct pitch_analyzer* s,
  struct algorithm_descriptor_boersma* ad,
  unsigned int ds,
  candidate_t* candidate) {
  const float t_max = ds * s->delta_t;
  const float r_max = candidate->amplitude;
  const float log_coef = (float)log2(s->minimal_frequency * t_max);

  candidate->weight = r_max - s->octave_cost * log_coef;
}

static double __tau(double x) {
  double x2 = x * x;
  return 1.0 / 4.0 * log(3*x2 + 6*x + 1) - sqrt(6) / 24 * log((x + 1 - sqrt(2 / 3)) / (x + 1 + sqrt(2 / 3)));
}

candidate_t* generate_boersma_unvoiced_candidates(
  struct pitch_analyzer* s,
  struct algorithm_descriptor_boersma_unvoiced* ad,
  float* frame_in) {
  // Alloc a new candidates (only one is used. others are just zeros)
  candidate_t* candidates = calloc(ad->parent.nb_candidates_per_step, sizeof(*candidates));

  // Its frequency is 0
  candidates->frequency = 0;
  // Its strength is function of the loc/glob peak
  const float numerator = fabs_max_arr(frame_in, ad->parent.frame_size) / s->global_absolute_peak;
  const float denominator = s->silence_threshold / (1.f + s->voicing_threshold);
  const float quotient = numerator / denominator;
  candidates->weight = s->voicing_threshold + (float)fmax(0.f, 2.f - quotient);

  return candidates;
}

// Interpolate with quadratic equation
static float __quadratic_method(uint k, float* X) {
    const float xl = (float)(k - 1);
    const float xc = (float)(k);
    const float xr = (float)(k + 1);
    const double yl = X[k - 1];
    const double yc = X[k];
    const double yr = X[k + 1];
    const double d2 = 2 * ((yr - yc) - (yl - yc) / (xl - xc)) / 2.0;
    const double d1 = (yr - yc) / (xr - xc) - 0.5 * d2 * (xr - xc);

    // (xe, ye) is the extremum point of the interpolated curve
    float xe, ye;
    if (d2) {
        xe = (float)(xc - d1 / d2);
        ye = (float)(yc + 0.5*d1*(xe - xc));
    } else { // degenerated case
        xe = (float)xc;
        ye = (float)yc;
    }
    return xe;
}

candidate_t* generate_boersma_candidates(
  struct pitch_analyzer* s,
  struct algorithm_descriptor_boersma* ad,
  float* frame_in) {
  // Lag values sorted by amplitude of the autocorrelation function
  unsigned int size_out;
  // Free last fft if required
  if (s->last_fft)
    free(s->last_fft);
  float* autocorrelation = compute_corrected_autocorrelation_and_fft(
    frame_in, ad->voiced_window, &ad->voiced_window_ac, &s->last_fft,
    ad->parent.frame_size, &size_out);
   // This value is dependent of the implementation autocorrelation's implementation.
   // It would be nice to have a better design when the fft is shared between algorithms
   // instead of retrieving it.
   s->last_fft_size = size_out * 4;

  // Candidates considered
  unsigned int nb_candidates = (ad->parent.nb_candidates_per_step > size_out) ?
    ad->parent.nb_candidates_per_step : size_out;
  candidate_t* candidates = calloc(nb_candidates, sizeof(*candidates));
  for (unsigned int ds = 1; ds + 1 < size_out; ds++) {
    const float v = autocorrelation[ds];

    //! Filter by local maximum
    if (v >= autocorrelation[ds - 1] && v >= autocorrelation[ds + 1]) {
      // Interpolate lag of maximal value
      float ds_new = __quadratic_method(ds, autocorrelation);
      // Todo : Also interpolate the amplitude and use it

      candidates[ds].frequency = 1.f / (ds_new * s->delta_t);
      candidates[ds].amplitude = autocorrelation[ds] / autocorrelation[0];
      // Compute the weigth of the candidate
      __boersma_compute_weigth(s, ad, ds, candidates + ds);
    }

    //! Filter by frequency
    const float freq = candidates[ds].frequency;
    if (freq < s->minimal_frequency || freq > s->maximal_frequency) {
      candidates[ds].frequency = 0;
      candidates[ds].amplitude = 0;
    }
  }
  // Sort by decreasing amplitude
  qsort(candidates, size_out, sizeof(*candidates), dsc_candidates_amplitude);

  // Free autocorrelation
  free(autocorrelation);

  // Store the candidates
  return candidates;
}

float* generate_frame_boersma(
  pitch_analyzer_t* s,
    algorithm_descriptor_boersma_t* adb,
  float* audio_buffer,
  unsigned int audio_buffer_index) {
    const algorithm_descriptor_t* ad = (algorithm_descriptor_t*)adb;
    // Number of samples on the right of audio_buffer_index required
    // to create a new frame.
    const unsigned int right_half_frame_size = (ad->frame_size % 2) ?
      ad->frame_size / 2 + 1 :
      ad->frame_size / 2;
  // In the case the frame end outside of the buffer, we say
  // we require more data.
  if (audio_buffer_index + right_half_frame_size > sb_count(s->audio_buffer))
    return NULL;

  int start_index = audio_buffer_index - ad->frame_size / 2;

  // In the case we are at the beginning of the stream
  if (start_index < 0) {
    start_index = 0;
    // We require more data!
    if (start_index + ad->frame_size > sb_count(s->audio_buffer))
      return NULL;
  }

  // Return the pointer to the begining of the frame.
  return audio_buffer + start_index;
}

float boersma_transition_cost(struct pitch_analyzer* s,
  const candidate_t *first, const candidate_t *second) {
  const float freq1 = first->frequency;
  const float freq2 = second->frequency;
  // Both unvoiced
  if (freq1 == 0 && freq2 == 0)
      return 0;
  // Voiced and unvoiced
  if (freq1 != freq2 && (freq1 == 0 || freq2 == 0))
      return s->voiced_unvoiced_cost;
  // When two frequencies candidate are compared
  return (float)(s->octave_jump_cost * fabs(log2(freq1 / freq2)));
 }
