#include "v2p.h"
#include "midi.h"
#include "tools.h"
#include "stretchy_buffer.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

static inline
double __inline__frequency_to_midi_number(double freq) {
  if (freq < 1)
    return 0;

  // A4 = 69 = 440Hz
  const double note_idx = log(freq / 440.0) / log(2.0) * 12;
  return 69 + note_idx;
}

double frequency_to_midi_number(double freq) {
  return __inline__frequency_to_midi_number(freq);
}

double midi_number_to_frequency(double number) {
  if (number < 1)
    return 0;

  // A4 = 69 = 440Hz
  return 440. * pow(2., (number - 69) / 12.);
}

float SYMPH_API* pitch_to_midi_numbers(float* pitch, uint length) {
  float* numbers = malloc(sizeof(*numbers) * length);

  for (uint i = 0; i < length; i++)
    numbers[i] = __inline__frequency_to_midi_number(pitch[i]);

  return numbers;
}

// Compute the difference between the current sample and the previous one
static float __local_diff(float* signal, uint i) {
  int index = i - 1;
  if (index < 0)
    index = 0;

  return fabsf(signal[index] - signal[i]);
}

// Compute the difference between the mean of the window and the considered point
static float __min_max_diff(float* window, uint window_length) {
  if (window_length < 1)
    return 0;

  float min, max;
  __get_min_max(window, window_length, &min, &max);

  return fabsf(min - max);
}

// Contain the class index of each sample from signal
uint* notes_segmentation_heuristic(float* numbers, uint length) {
  // Create result array initialized to a unique class.
  uint* segment_array = calloc(length, sizeof(*segment_array));
  if (!segment_array)
    return NULL;

  // This value control the length of the window considered
  // This value (1 is 10ms) was selected empirically.
  // Further research could be done to adjust it.
  const uint window_length = 3;
  // This is the maximum distance (in midi numbers) allowed betwen extremal points in
  // a window. The value 1.f is a half tone.
  const float pitch_threshold = 1.f / 2.5f;
  // This is a constant threshold. We now use a dynamic threshold.
  const float pitch_local_threshold = 1.f / 5.f;

  // For each sample in the signal
  uint current_class = 0;
  for (uint i = 0; i < length; i++) {
    // Compute the location of the begining of the window we consider.
    int index = i - window_length + 1;
    if (index < 0)
      index = 0;
    if (index + window_length >= length)
      index = length - window_length;

    // Check if the points in the window seams close to each others.
    float mmd = 0;
    if ((mmd = __min_max_diff(numbers + index, window_length)) >= pitch_threshold &&
        __local_diff(numbers, i) > mmd / 1.5f) // Dynamic threshold
      current_class++;

    // Set the class of the current sample
    segment_array[i] = current_class;
  }

  return segment_array;
}

// Signed distance to grid
float distance_to_grid(float midi_number) {
  if (midi_number < 1)
    return 0;

  // Signed distance : (0, 0.5] is above the note,
  //                   (0, -0.5] is below.
  return (float)(midi_number - round(midi_number));
}

static sb_note add_note(sb_note a, float note_number, float position, float duration) {
  midi_note_data_t note;

  note.note_number = note_number;
  note.position = position;
  note.duration = duration;
  // Silence
  if (note_number == 0)
    note.velocity = 0;
  // Default velocity
  else
    note.velocity = 0x40;

  sb_push(a, note);

  return a;
}

midi_note_data_t* merge_overlapping_notes(const midi_note_data_t* midi_array, uint length, uint* nb_notes) {
  *nb_notes = 0;
  if (length < 1)
    return NULL;
  // New array
  midi_note_data_t* notes = malloc(sizeof(*notes) * sb_count(midi_array));

  const uint minimal_note_length = 6;
  uint i = 1;
  notes[0] = midi_array[0];
  
  // Skip glitch note at beginning
  for(; midi_array[i - 1].duration < minimal_note_length && i < length; i++)
    notes[0] = midi_array[i];

  // Copy each note if relevant
  midi_note_data_t* last_note = notes;
  uint counter = 1;
  for (;i < length; i++) {
    // Case whene two notes should be merged
    const float last_note_end = last_note->duration + last_note->position;
    if (last_note_end >= midi_array[i].position &&
      (last_note->duration < 10 || midi_array[i].duration < 10) &&// Duration is small
      last_note->velocity == midi_array[i].velocity &&
      last_note->note_number == midi_array[i].note_number) {
      last_note->duration = midi_array[i].position - last_note->position + midi_array[i].duration;
    }
    // Case when the note should be added
    else {
      // Skip note that are too short !!!
      if (midi_array[i].duration < minimal_note_length) // heuristic : Glitch are note with length below 60ms
        continue;
      // Add the new note
      last_note++;
      counter++;
      (*last_note) = midi_array[i];
    }
  }

  *nb_notes = counter;
  return notes;
}

//! Structure for the creation of notes in add_note_from_segment_using_mean_pitch
struct note_info {
  float note_sum;
  float note_weigth;
  float variable_weight;
  uint note_count;
};

static void init_note_info(struct note_info* ni)
{
  ni->note_sum = 0;
  ni->note_weigth = 0;
  ni->variable_weight = 0.1f;
  ni->note_count = 0;
}

void add_note_from_segment_using_buckets(sb_note* midi_array, float position, float* midi_numbers, uint note_len)
{
  float buckets[129];
  memset(buckets, 0, 129 * sizeof(float));

  // Counts the number of midi number sample that belongs to each bucket
  for (uint i = 0; i < note_len; i++) {
    int b_num = round(midi_numbers[i]);
    if (b_num > 128 || b_num < 0)
      fprintf(stderr, "Warning: the range value for a midi note is [0, 128]. Got %f instead\n", midi_numbers[i]);
    else
      buckets[b_num] += 1;
  }

  uint selection = fabs_argmax(buckets, 129);

  const float duration = (float)(note_len);
  const float note_number = (float)(selection);
  *midi_array = add_note(*midi_array, note_number, position, duration);
}

static void add_note_from_segment_using_mean_pitch(sb_note* midi_array, float position, float* midi_numbers, uint note_len)
{
  struct note_info note_info;
  init_note_info(&note_info);

  for (uint i = 0; i < note_len; i++) {
    // Check values befor usage
    if (midi_numbers[i] < 0 || midi_numbers[i] > 128) {
      fprintf(stderr, "Warning: the range value for a midi note is [0, 128]. Got %f instead\n", midi_numbers[i]);
      continue;
    }
    
    note_info.note_sum += midi_numbers[i] * note_info.variable_weight; // Add dynamic weighting
    note_info.note_weigth += note_info.variable_weight;
    note_info.variable_weight += 0.10f;
    if (note_info.variable_weight > 1.f)
      note_info.variable_weight = 1.f;
    note_info.note_count++;
  }

  const float duration = (float)(note_info.note_count);
  const float note_number = note_info.note_sum / note_info.note_weigth;
  *midi_array = add_note(*midi_array, note_number, position, duration);
}

midi_note_data_t SYMPH_API* midi_numbers_to_notes(float* midi_numbers, uint length, uint* nb_notes) {
  // If there is no midi numbers, we have no notes
  if (length < 1)
    return NULL;

  sb_note midi_array = NULL;

  // Segmentation
  uint* segment_array = notes_segmentation_heuristic(midi_numbers, length);

  // Create note
  uint current_segment = segment_array[0];

  uint note_start = 0;
  for (uint i = 0; i < length; i++) {
    // New segment
    if (current_segment != segment_array[i]) {
      // Change current segment
      current_segment = segment_array[i];

      // Calculates the note with the medium of the segment
      add_note_from_segment_using_buckets(&midi_array, note_start, &midi_numbers[note_start], i - note_start);

      note_start = i;
    }
  }
  
  // Add last note
  add_note_from_segment_using_buckets(&midi_array, note_start, &midi_numbers[note_start], length - note_start);

  // Clean memory
  free(segment_array);

  // Merge overlapping notes into a single one
  midi_note_data_t* midi = merge_overlapping_notes(midi_array, sb_count(midi_array), nb_notes);
  sb_free(midi_array);

  return midi;
}
