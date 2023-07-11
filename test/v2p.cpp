#include "lib/TestHarness.hpp"
#include "v2p.h"
#include "tools.h"
#include "stretchy_buffer.h"
#include "boersma.h"
#include "maxfreq.h"
#include "midi.h"

#include <vector>

TEST (SYMP, v2p_scenario_feed_audio_buffer)
{
    // Input audio and settings
    unsigned int frame_size = 2048;
    std::vector<float> buffer(48000 * 2); //2s of audio

    // Initialize structure and algorithm
    struct pitch_analyzer* s = v2p_new(0);
    s->zero_padding = (rand() % 3000);
    v2p_reset(s);

    // Sinusoid at 220Hz
    for(unsigned int i = 0; i < buffer.size(); i++)
      buffer[i] = (float)sin(i * 150 * 2 * M_PI / s->sampling_rate);

    // Feed samples by chunks of 512, because why not
    const unsigned int chunk_size = 512;
    unsigned int i;
    for (i = 0; i + chunk_size < buffer.size(); i += chunk_size)
      v2p_add_samples(s, &buffer[i], chunk_size);
    if (i < buffer.size())
        v2p_add_samples(s, &buffer[i], buffer.size() - i);

    // Assert the audio buffer contain all the samples (with padding into account)
    for (unsigned int i = 0; i < buffer.size(); i++)
      CHECK_DOUBLES_EQUAL(buffer[i], s->audio_buffer[i + s->zero_padding]);

    // Free engine
    v2p_delete(s);
}

static float
__fake_transition_cost(struct pitch_analyzer*, candidate_t* first, candidate_t* second) {
  const float diff = (float)fabs(first->frequency - second->frequency);
  if (diff < 10)
    return diff;
  return 1000.f;
}

TEST(SYMP, viterbi_path_cost_on_obvious_candidates)
{
  // Initialize structure and algorithm
  struct pitch_analyzer* s = v2p_new(0.f);
  s->compute_transition_cost = __fake_transition_cost;
  s->nb_candidates_per_step = 3;

  // Input audio and settings
  std::vector<candidate_t> path(s->nb_candidates_per_step, {0, 1.f});

  // 1rst STEP
  path[0].frequency = 100;
  path[1].frequency = 200;
  path[2].frequency = 300;

  // Run algorithm by hand
  sb_concat(s->candidates, &path[0], s->nb_candidates_per_step);
  s->number_of_timesteps++;
  update_viterbi_path(s);

  // Check the path cost of freq10
  CHECK_DOUBLES_EQUAL(-1, s->path_costs[0])
  // Check the path cost of freq20
  CHECK_DOUBLES_EQUAL(-1, s->path_costs[1])
  // Check the path cost of freq30
  CHECK_DOUBLES_EQUAL(-1, s->path_costs[2])

  // 2nd STEP
  path[0].frequency = 205;
  path[1].frequency = 301;
  path[2].frequency = 105;

  // Run algorithm by hand
  sb_concat(s->candidates, &path[0], s->nb_candidates_per_step);
  s->number_of_timesteps++;
  update_viterbi_path(s);

  // Check the path cost of freq20
  CHECK_DOUBLES_EQUAL(5-2, s->path_costs[0])
  // Check the path cost of freq30
  CHECK_DOUBLES_EQUAL(1-2, s->path_costs[1])
  // Check the path cost of freq10
  CHECK_DOUBLES_EQUAL(5-2, s->path_costs[2])

  // 3rd STEP
  path[0].frequency = 302;
  path[1].frequency = 210;
  path[2].frequency = 110;

  // Run algorithm by hand
  sb_concat(s->candidates, &path[0], s->nb_candidates_per_step);
  s->number_of_timesteps++;
  update_viterbi_path(s);

  // Check the path cost of freq30
  CHECK_DOUBLES_EQUAL(2-3, s->path_costs[0])
  // Check the path cost of freq20
  CHECK_DOUBLES_EQUAL(10-3, s->path_costs[1])
  // Check the path cost of freq10
  CHECK_DOUBLES_EQUAL(10-3, s->path_costs[2])

  // Use the path generator to retrive the path of freq30
  float* p = v2p_compute_path(s);

  // Check the path is 300->301->302
  CHECK_DOUBLES_EQUAL(300, p[0]);
  CHECK_DOUBLES_EQUAL(301, p[1]);
  CHECK_DOUBLES_EQUAL(302, p[2]);

  //Free memory
  free(p);
  v2p_delete(s);
}


TEST(SYMP, median)
{
  std::vector<float> input {9, 2, 2, 8, 2, 1, 2, 2, 2, 2, 0, 2, 9};
  const uint window_size = 3;

  float* output = median_filter(&input[0], input.size(), window_size);

  for (int i = 1; i < input.size() - 1; i++)
    if (input[i] == 0) {
      CHECK_DOUBLES_EQUAL(0, output[i]); // Are zero preserved
    }
    else {
      CHECK_DOUBLES_EQUAL(2, output[i]); // Is median computed
    }
  CHECK_DOUBLES_EQUAL(9, output[0]);
  CHECK_DOUBLES_EQUAL(9, output[input.size() - 1]);

  v2p_ptr_free(output);
}


TEST(SYMP, mean)
{
  std::vector<float> input {2, 4, 4, 4, 0};
  const uint window_size = 3;

  float* output = mean_filter(&input[0], input.size(), window_size);

  CHECK_DOUBLES_EQUAL(3, output[0]);
  CHECK_DOUBLES_EQUAL(10.f/3, output[1]);
  CHECK_DOUBLES_EQUAL(4, output[2]);
  CHECK_DOUBLES_EQUAL(4, output[3]);
  CHECK_DOUBLES_EQUAL(0, output[4]);

  v2p_ptr_free(output);
}


TEST (SYMP, v2p_memory_sanity_regression)
{
    // Input audio and settings
    unsigned int frame_size = 2048;
    std::vector<float> buffer(4096);

    // Initialize structure and algorithm
    pitch_analyzer_t* s = v2p_new(0);
    auto b = boersma_new(0, 0);
    auto u = boersma_unvoiced_new(0);
    auto f = maxfreq_new(0);
    v2p_register_algorithm(s, (algorithm_descriptor*)f);
    v2p_register_algorithm(s, (algorithm_descriptor*)u);
    v2p_register_algorithm(s, (algorithm_descriptor*)b);

    for(unsigned int i = 0; i < buffer.size(); i++)
        buffer[i] = (float)random();

    for (int i = 0; i < 5; i ++) {
        v2p_add_samples(s, &buffer[0], (uint)buffer.size());
        auto path_len = v2p_path_len(s);
        auto pitch = v2p_compute_path(s);
        auto numbers = pitch_to_midi_numbers(pitch, path_len);
        uint nb_notes = 0;
        auto notes = midi_numbers_to_notes(numbers, path_len, &nb_notes);

        if (path_len > 0) {
            auto a = pitch[0];
            auto b = numbers[0];
        }
        if (nb_notes > 0)
            auto c = notes[0];

        v2p_ptr_free(notes);
        v2p_ptr_free(numbers);
        v2p_ptr_free(pitch);
    }


    // Free engine
    v2p_delete(s);
    boersma_delete(b);
    boersma_unvoiced_delete(u);
    maxfreq_delete(f);
}
