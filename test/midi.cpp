#include "lib/TestHarness.hpp"
#include "midi.h"
#include "stretchy_buffer.h"

#include <vector>
#include <algorithm>

TEST (MIDI, frequency_to_midi_note) {
  float freq[] = {
    130.81,
    440.00,
    466.16,
    493.88,
    523.25,
    554.37,
    587.33,
    622.25,
    659.26,
    698.46,
    739.99,
    783.99,
    830.61,
    880.00
  };

  uint number[] = {
    48, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
  };

  for (uint i = 0; i < sizeof(number) / sizeof(*number); i++)
    CHECK_LONGS_EQUAL(number[i], round(frequency_to_midi_number(freq[i])));
}

TEST (MIDI, signal_segmentation)
{
    std::vector<float> signal(1024, 0);

    // Select keyboard key between 440 and 880 Hz
    uint i = 0;
    uint nb_segments = 0;
    float last_freq = 0;
    while (i < signal.size()) {
      // Get a new frequency
      const double k = round((rand() * 12.0)/(double)RAND_MAX);
      const float freq = (float)(440 * pow(2, k / 12.f));
      if (freq == last_freq)
        continue;

      // New segment
      const uint duration = (uint)(rand() * 400 /(double)RAND_MAX) + 80;
      uint end = i + duration;
      if (end - signal.size() < 80)
          end = signal.size();
      last_freq = freq;
      nb_segments++;

      // Feed data with noise
      while(i < signal.size() && i < end)
        signal[i++] = freq + (float)(rand() * 2.0 / RAND_MAX);
    }

    // Compute Segmentation and count number of segments
    float* numbers = pitch_to_midi_numbers(&signal[0], signal.size());
    uint* segment_array = notes_segmentation_heuristic(numbers, signal.size());
    uint nb_computed_segments = 1;
    uint last_segment_class = segment_array[0];
    for (uint j = 0; j < signal.size(); j++) {
      if (last_segment_class != segment_array[j])
        nb_computed_segments++;
      last_segment_class = segment_array[j];
    }
    free(segment_array);

    // Check the number of segments
    CHECK_LONGS_EQUAL(nb_segments, nb_computed_segments);

    // Compute midi notes
    uint nb;
    midi_note_data_t* notes = midi_numbers_to_notes(numbers, signal.size(), &nb);

    CHECK_LONGS_EQUAL(nb_segments, nb);

    free(notes);
    free(numbers);
}

// Non regression test for a buffer overflow
TEST(MIDI, add_note_from_segment_using_buckets) {
    float midi_numbers[] = {
      57.6,
      56.0,
      58.3,
      57.3,
      58.4,
      0.0,
      128.9,
      128.0
    };

    sb_note midi_array = NULL;

    add_note_from_segment_using_buckets(&midi_array, 0, midi_numbers, 8);

    CHECK_LONGS_EQUAL(58.0, midi_array[0].note_number);
    sb_free(midi_array);
}
