#include "v2p.h"
#include "tools.h"
#include "boersma.h"
#include "stretchy_buffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>

void v2p_init(pitch_analyzer_t* s, float timesteps) {
  // Set everything to 0
  memset(s, 0, sizeof(*s));

  // Default values
  if (timesteps == 0)
    timesteps = 0.01f; // 10ms

  // Some presets
  s->frame_time_step = timesteps;
  s->minimal_frequency = 20;
  s->maximal_frequency = 800;
  s->octave_cost = 0.02f; // 0.04 would be a 20% criterion and fit human hear. (def: 0.01)
  s->octave_jump_cost = 0.2f;
  s->voiced_unvoiced_cost = 0.2f; // def: 0.2f
  s->silence_threshold = 0.15f; // def: 0.05
  s->voicing_threshold = 0.4f;
  s->zero_padding = 2048;
  s->initial_absolute_peak_coeff = 1.0f;
  s->minimal_note_length = 6; // Keep note of length > this value. (unit in samples)

  // Some parameters that can be changed
  s->sampling_rate = 48000;

  // Allocated resources
  s->audio_buffer = NULL;
  s->algorithm_descriptors = NULL;
  s->compute_transition_cost = (coster_t)boersma_transition_cost;
  s->path_costs = NULL;

  v2p_reset(s);
}

pitch_analyzer_t* v2p_new(float timesteps) {
  pitch_analyzer_t *s;

  s = calloc(1, sizeof(pitch_analyzer_t));
  v2p_init(s, timesteps);

  return s;
}

void SYMPH_API v2p_delete(pitch_analyzer_t* s) {
  s->candidates = sb_free(s->candidates);
  s->audio_buffer = sb_free(s->audio_buffer);
  s->path_indexes = sb_free(s->path_indexes);
  if(s->path_costs)
    s->path_costs = (free(s->path_costs), NULL);
  if (s->last_fft)
    s->last_fft = (free(s->last_fft), NULL);
  free(s);
}

void v2p_reset(pitch_analyzer_t* s) {
  // Some computed values
  s->delta_t = 1.f / s->sampling_rate;
  // Size of a step
  s->frame_step_size = (int)(s->frame_time_step * s->sampling_rate);

  // Remove produced data
  s->candidates = sb_free(s->candidates);
  s->audio_buffer = sb_free(s->audio_buffer);
  s->number_of_timesteps = 0;
  s->audio_buffer_index = 0;
  s->path_indexes = sb_free(s->path_indexes);
  if(s->path_costs)
    s->path_costs = (free(s->path_costs), NULL);
  // Add padding
  for (uint i = 0; i < s->zero_padding; i++)
    sb_push(s->audio_buffer, 0);
  s->audio_buffer_index = s->zero_padding;
}

// Assert length > 0
unsigned int fargmin(float* a, uint length) {
  float min = a[0];
  unsigned int arg = 0;
  for (unsigned int i = 0; i < length; i++)
    if (a[i] < min) {
      min = a[i];
      arg = i;
    }
  return arg;
}

void update_viterbi_path(pitch_analyzer_t* s) {
  // Last candidates:
  candidate_t* candidates = s->candidates + sb_count(s->candidates) - s->nb_candidates_per_step;
  candidate_t* old_candidates = s->candidates + sb_count(s->candidates) - 2 * s->nb_candidates_per_step;

  // We require at least two timesteps to access old_condidates
  if (s->number_of_timesteps <= 1) {
    if (s->path_costs) // we should never enter this if
      free(s->path_costs);
	// First allocation of path_costs
    // and corresponding path_indexes
	s->path_costs = malloc(s->nb_candidates_per_step * sizeof(float));
    s->path_indexes = sb_free(s->path_indexes);
    sb_reserve(s->path_indexes, s->nb_candidates_per_step);
    for (uint i = 0; i < s->nb_candidates_per_step; i++) {
      (s->path_costs)[i] = -candidates[i].weight;
      (s->path_indexes)[i] = i;
    }
    return;
  }

  // Allocate the local version of path_costs and path_indexes
  float* new_path_costs = calloc(s->nb_candidates_per_step, sizeof(float));
  uint* new_path_indexes = calloc(s->nb_candidates_per_step, sizeof(uint));
  // For each candidate
  for (uint candidate_idx = 0; candidate_idx < s->nb_candidates_per_step; candidate_idx++) {
    // new frequency
  	candidate_t* cdt2 = &candidates[candidate_idx];

    sb_float scores = NULL;
    // For each path (0 is unvoiced)
    for (uint path_idx = 0; path_idx < s->nb_candidates_per_step; path_idx++) {
      // previous frequency already in path
  		candidate_t* cdt1 = &old_candidates[path_idx];

      // Compute the score and add it to the array
      const float new_cost = s->path_costs[path_idx] +
        s->compute_transition_cost(s, cdt1, cdt2) +
        -candidates[candidate_idx].weight;
      sb_push(scores, new_cost);
    }

    const uint arg_min = fargmin(scores, sb_count(scores));
    new_path_costs[candidate_idx] = scores[arg_min];
    new_path_indexes[candidate_idx] = arg_min;
    sb_free(scores);
  }

  symp_swap(s->path_costs, new_path_costs);
  sb_concat(s->path_indexes, new_path_indexes, s->nb_candidates_per_step);
  free(new_path_costs);
  free(new_path_indexes);
}

//! Called when the audio buffer of a pitch_analyzer changed.
void v2p_audio_buffer_changed(pitch_analyzer_t* s) {
  // Wait for at least some data
  if (sb_count(s->audio_buffer) < 1024)
      return;

  // Initialize global_absolute_peak the first time with initial_absolute_peak_coeff* the maximum of the first frame
  if (s->global_absolute_peak <= 0) {
    const unsigned int length = _min(sb_count(s->audio_buffer), s->zero_padding + 1024);
    s->global_absolute_peak =
      fabs_max_arr(s->audio_buffer, length) * s->initial_absolute_peak_coeff;
  }
  // Update global Absolute Peak
  const float loc_abs_peak = fabs_max_arr(s->audio_buffer, sb_count(s->audio_buffer));
  s->global_absolute_peak = _max(loc_abs_peak, s->global_absolute_peak);

  // While something remind in the buffer
  while(s->audio_buffer_index < sb_count(s->audio_buffer)) {
    // Check if all algorithm can compute a new frame for current index
    bool data_available = true;
    struct algorithm_descriptor* ad = s->algorithm_descriptors;
    while(data_available && ad) {
      data_available = data_available && ad->generate_frame(
        s, ad, s->audio_buffer, s->audio_buffer_index);
      ad = ad->next;
    }
    if (!data_available)
      break;

    // Run all the algorithm on their frame
    ad = s->algorithm_descriptors;
    while (ad) {
      // Generate frame
      float* frame = ad->generate_frame(
        s, ad, s->audio_buffer, s->audio_buffer_index);
      // Generate candidates
		candidate_t* candidates =
			ad->generate_candidates(s, ad, frame);
      // Add the candidates to the candidate buffer
      sb_concat(s->candidates, candidates, ad->nb_candidates_per_step);
      // Clean memory and go to the next algorithm
      v2p_ptr_free(candidates);
      ad = ad->next;
    }

    // Go to the next collection of frames.
    s->audio_buffer_index += s->frame_step_size;
    s->number_of_timesteps++;

    //! Construct the coeffecients used to build the path through candidates
    update_viterbi_path(s);
  }
}

void v2p_add_samples(pitch_analyzer_t* s,
  const float* samples_in, unsigned int size_in) {
    // Reserve more memory
    sb_concat(s->audio_buffer, samples_in, size_in);

    // Actualise inline computation of the pitch
    v2p_audio_buffer_changed(s);
}

void v2p_register_algorithm(pitch_analyzer_t* s,
  struct algorithm_descriptor* ad) {
  ad->next = s->algorithm_descriptors;
  s->algorithm_descriptors = ad;
  s->nb_candidates_per_step += ad->nb_candidates_per_step;
}

unsigned int v2p_nb_candidates_generated(pitch_analyzer_t*s) {
  return sb_count(s->candidates);
}

float* v2p_compute_path(pitch_analyzer_t* s) {
  if (!s->path_costs || !s->nb_candidates_per_step)
    return NULL;

  float* frequency_history =
    malloc(s->number_of_timesteps * sizeof(*frequency_history));

  unsigned int best_candidate = fargmin(s->path_costs, s->nb_candidates_per_step);

  for (uint i = s->number_of_timesteps - 1; i > 0; i--) {
      const uint candidates_idx = i * s->nb_candidates_per_step;
      frequency_history[i] = s->candidates[candidates_idx + best_candidate].frequency;

      const uint path_idx = (i - 1) * s->nb_candidates_per_step;
      best_candidate = s->path_indexes[path_idx + best_candidate];
  }
  frequency_history[0] = s->candidates[best_candidate].frequency;

  return frequency_history;
}

void* v2p_ptr_free(void* ptr) {
  free(ptr);
  return NULL;
}

static int __cmp_floats(const void * a, const void * b)
{
  const float diff = *(float*)a - *(float*)b;
  if(diff > 0)
      return 1;
  if(diff < 0)
      return -1;
  return 0;
}

static float __get_median(float* window, uint window_size) {
  // Sort the window
  qsort(window, window_size, sizeof(float), __cmp_floats);
  // Odd
  if (window_size % 2 == 1)
    return window[window_size / 2];
  // Even
  else
    return (window[window_size / 2 - 1] + window[window_size / 2]) / 2.f;
}

float* median_filter(float* input, uint length, uint window_size) {
  float *window = calloc(sizeof(*window), window_size);
  if (!window)
    return NULL;
  float *out = malloc(sizeof(*out) * length);
  if (!out)
    return NULL;

  // handle the case when window_size is even
  const uint half_left = window_size / 2;
  const uint half_right = window_size - half_left;

  uint i = 0;
  // Begining of array
  for (; i < half_left; i++)
    out[i] = input[i];
  // Ensure window is contained in dataset
  for (i = half_left; i + half_right < length; i++) {
    // Fill the window
    memcpy(window, input + i - half_left, window_size * sizeof(float));
    // Store the median
    out[i] = __get_median(window, window_size);
    // Preserve silences
    if (input[i] == 0)
      out[i] = 0;
  }
  // End of array
  for (; i < length; i++)
    out[i] = input[i];

  free(window);
  return out;
}

float* mean_filter(float* input, uint length, uint window_size) {
  float *out = malloc(sizeof(*out) * length);
  if (!out)
    return NULL;

  // handle the case when window_size is even
  const int half_left = window_size / 2;
  const int half_right = window_size - half_left;

  for (uint i = 0; i < length; i++) {
    // Compute mean
    float mean = 0.f;
    int nb = 0;
    for (int j = -half_left; j < half_right; j++)
      // Ensure window sample is contained in buffer
      if (i + j >= 0 && i + j < length && input[i + j] != 0) {
        mean += input[i + j];
        nb++;
      }
    out[i] = mean / nb;

    // Preserve silences
    if (input[i] == 0)
      out[i] = 0;
  }
  return out;
}

unsigned int v2p_path_len(pitch_analyzer_t* s) {
    return s->number_of_timesteps;
}
