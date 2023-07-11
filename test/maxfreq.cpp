#include "lib/TestHarness.hpp"
#include "maxfreq.h"
#include "boersma.h"
#include "v2p.h"
#include "tools.h"

#include <vector>
#include <algorithm>
#include <cstdlib>

TEST (Maxfreq, generate_maxfreq_candidates_should_return_frequency_from_sin)
{
  // Input frame
  unsigned int frame_size = 2048;
  std::vector<float> frame_in(frame_size);

  // Initialize structure and algorithm
  struct pitch_analyzer *s;
  struct algorithm_descriptor_boersma *b;
  struct algorithm_descriptor_maxfreq *f;
  s = v2p_new(0);
  b = boersma_new(frame_size, 0);
  f = maxfreq_new(frame_size);

  // Sinusoid at 900Hz
  for(unsigned int i = 0; i < frame_size; i++)
    frame_in[i] = (float)sin(i * 1200 * 2 * M_PI / s->sampling_rate);

  // TODO: Min freq and max freqs
  CHECK(s->minimal_frequency < 50);
  CHECK(s->maximal_frequency > 200);

  // Run the algorithm on the frame.
  candidate_t* candidates;
  candidates = generate_boersma_candidates(s, b, &frame_in[0]);
  v2p_ptr_free(candidates);
  candidates = generate_maxfreq_candidates(s, f, &frame_in[0]);

  // It should return some candidates
  CHECK(b->parent.nb_candidates_per_step >= 1);

  // The candidates should be multiple of the frequency of sin.
  CHECK(candidates[0].amplitude > 0);
  CHECK(fabs(1200 - candidates[0].frequency) < 10);

  // Free memory
  v2p_ptr_free(candidates);
  v2p_delete(s);
  boersma_delete(b);
  maxfreq_delete(f);
}
