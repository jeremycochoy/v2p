#include "lib/TestHarness.hpp"
#include "boersma.h"
#include "v2p.h"
#include "tools.h"

#include <vector>
#include <algorithm>
#include <cstdlib>

TEST (Boersma, dsc_candidates_amplitude_sort_on_sample)
{
  unsigned int size = 4;
  std::vector<candidate_t> tab(size);

  tab[0].amplitude = 5;
  tab[1].amplitude = 200;
  tab[2].amplitude = 1;
  tab[3].amplitude = 8;

  qsort(&tab[0], size, sizeof(candidate_t), dsc_candidates_amplitude);

  CHECK_DOUBLES_EQUAL(200, tab[0].amplitude);
  CHECK_DOUBLES_EQUAL(8,   tab[1].amplitude);
  CHECK_DOUBLES_EQUAL(5,   tab[2].amplitude);
  CHECK_DOUBLES_EQUAL(1,   tab[3].amplitude);
}

// Non regression test
TEST (Boersma, dsc_candidates_amplitude_sort_on_sample_bis)
{
  unsigned int size = 30;
  std::vector<candidate_t> tab(size);
  tab[10].amplitude = 200;
  tab[10].frequency = 1;
  tab[20].amplitude = 200;
  tab[20].frequency = 2;

  qsort(&tab[0], size, sizeof(candidate_t), dsc_candidates_amplitude);

  CHECK_DOUBLES_EQUAL(200, tab[0].amplitude);
  CHECK_DOUBLES_EQUAL(200, tab[1].amplitude);
  CHECK_DOUBLES_EQUAL(0,   tab[2].amplitude);
  CHECK_DOUBLES_EQUAL(0,   tab[3].amplitude);
}

TEST (Boersma, generate_boersma_candidates_should_return_frequency_from_sin)
{
  // Input frame
  unsigned int frame_size = 2048;
  std::vector<float> frame_in(frame_size);

  // Initialize structure and algorithm
  struct pitch_analyzer *s;
  struct algorithm_descriptor_boersma *b;
  s = v2p_new(0);
  b = boersma_new(frame_size, 0);

  // Sinusoid at 150Hz
  for(unsigned int i = 0; i < frame_size; i++)
    frame_in[i] = (float)sin(i * 150 * 2 * M_PI / s->sampling_rate);

  // Min freq and max freq corects
  CHECK(s->minimal_frequency < 50);
  CHECK(s->maximal_frequency > 200);

  // Run the algorithm on the frame.
  candidate_t* candidates = generate_boersma_candidates(s, b, &frame_in[0]);

  // It should return some candidates
  CHECK(b->parent.nb_candidates_per_step >= 1);

  // The candidates should be multiple of the frequency of sin.
  for (unsigned int i = 0; i < b->parent.nb_candidates_per_step; i++) {
     const uint freq = (uint)round(candidates[i].frequency);
    CHECK(
      150 == freq ||
      75  == freq ||
      50  == freq
    );
    CHECK(candidates[i].amplitude > 0);
  }

  // Free memory
  v2p_ptr_free(candidates);
  v2p_delete(s);
  boersma_delete(b);
}

#include "stretchy_buffer.h"

TEST (Boersma, v2p_scenario_boersma)
{
  // Input audio and settings
  unsigned int frame_size = 2048;
  std::vector<float> buffer(48000 * 2); //2s of audio

  // Initialize structure and algorithm
  struct pitch_analyzer* s = v2p_new(0);
  struct algorithm_descriptor_boersma* adb = boersma_new(frame_size, 0);
  struct algorithm_descriptor_boersma_unvoiced* adu = boersma_unvoiced_new(frame_size);
  s->zero_padding = 0; // Unactivate zero padding
  v2p_reset(s);
  // Connect algorithms
  v2p_register_algorithm(s, (algorithm_descriptor*)adb);
  v2p_register_algorithm(s, (algorithm_descriptor*)adu);

    // Sinusoid at 220Hz
    for(unsigned int i = 0; i < buffer.size(); i++)
      buffer[i] = (float)sin(i * 150 * 2 * M_PI / s->sampling_rate);

    // Feed samples
    v2p_add_samples(s, &buffer[0], (unsigned int)buffer.size());

    // Check we have nb_candidates_per_step * number_of_frames candidates.
    const unsigned int number_of_frames = s->audio_buffer_index / s->frame_step_size;
    CHECK_LONGS_EQUAL(s->nb_candidates_per_step * number_of_frames,
      sb_count(s->candidates));

    // Compute paths and check its value
    float* path = v2p_compute_path(s);
    for (uint i = 0; i < s->number_of_timesteps; i++)
      CHECK_LONGS_EQUAL(150, (long)round(path[i]))
    free(path);

    v2p_delete(s);
    boersma_delete(adb);
    boersma_unvoiced_delete(adu);
}

TEST(Boersma, transition_cost) {
  // We have our different candidates:
  candidate_t a, b, z;
  pitch_analyzer_t* s;

  s = v2p_new(0);
  a.weight = b.weight = z.weight = 1.f;
  a.frequency = 110; // voiced
  b.frequency = 220; // voiced
  z.frequency = 0;   // unvoiced

  const float voiced_unvoiced = boersma_transition_cost(s, &a, &z);
  const float voiced_octave = boersma_transition_cost(s, &a, &b);
  const float octave_octave = boersma_transition_cost(s, &b, &b);
  const float unvoiced_unvoiced = boersma_transition_cost(s, &z, &z);
  const float voiced_voiced = boersma_transition_cost(s, &a, &a);

  // Check that a transition is always worst than staying on place
  CHECK(voiced_unvoiced > voiced_voiced);
  CHECK(voiced_unvoiced > unvoiced_unvoiced);
  CHECK(voiced_octave > voiced_voiced);
  CHECK(voiced_octave > octave_octave);

  v2p_delete(s);
}

// Regression test
TEST (Boersma, scenario_boersma_unvoiced_huge_data)
{
  // Input audio and settings
  unsigned int frame_size = 2048;
  std::vector<float> buffer(961281); //2s of audio

  // Initialize structure and algorithm
  struct pitch_analyzer* s = v2p_new(0);
  struct algorithm_descriptor_boersma_unvoiced* adu = boersma_unvoiced_new(frame_size);
  v2p_reset(s);
  // Connect algorithms
  v2p_register_algorithm(s, (algorithm_descriptor*)adu);

  // Sinusoid at 220Hz
  for(unsigned int i = 0; i < buffer.size(); i++)
    buffer[i] = (float)sin(i * 150 * 2 * M_PI / s->sampling_rate);

  // Feed samples
  v2p_add_samples(s, &buffer[0], (unsigned int)buffer.size());

  // Compute paths and check its value
  float* path = v2p_compute_path(s);
  for (uint i = 0; i < s->number_of_timesteps; i++)
    CHECK_LONGS_EQUAL(0, (long)round(path[i]))
  free(path);

  v2p_delete(s);
  boersma_unvoiced_delete(adu);
}
