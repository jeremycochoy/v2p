#ifndef MIDI_H_
#define MIDI_H_

#include "v2p.h"
#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct midi_note_data {
    float note_number; // MIDI note number (see specs): 69 = A4 = 440Hz
    uint velocity; // 40 is default velocity, 0-127
    float duration; // In beats
    float position; // In beats
};

typedef struct midi_note_data midi_note_data_t;
typedef midi_note_data_t* sb_note;


//! Signed distance of the given midi note number to the piano keyboard.
float SYMPH_API distance_to_grid(float midi_number);
//! Convert a stream of frequency to a stream of notes
float SYMPH_API* pitch_to_midi_numbers(float* pitch, uint length);
//! Convert a stream of midi numbers to an array of midi_notes.
//!
//! @param[out] nb_notes Number of notes recorvered
//! @return An array of midi_note_data_t of length nb_notes
midi_note_data_t SYMPH_API* midi_numbers_to_notes(float* numbers, uint length, uint* nb_notes);
//! Adds a note from a segment by fitting the segment's numbers into buckets
void add_note_from_segment_using_buckets(sb_note* midi_array, float position, float* midi_numbers, uint note_len);
//! Take a stream of midi note number with a sample each 10ms and
//! segment it to midi notes.
//! The return array is a mapping from each sample to the note number.
//! For example the signal [69, 69, 69, 30, 30, 30] will be break into
//! two notes, and the algorithm will return [0, 0, 0, 1, 1, 1].
//!
//! @param midi_numbers A pitch path converted to midi notes with pitch_to_notes.
//! @return An array of class: Each `note[i]` is associated to a class index.
uint SYMPH_API* notes_segmentation_heuristic(float* midi_numbers, uint length);
//! Convert a frequency to its midi note number value.
//! The frequency 440Hz of A4 is converted to 69.
//! Then comes A4#->70, B4->71, etc..
double SYMPH_API frequency_to_midi_number(double frequency);
//! Convert a midi note number back to the frequency value.
//! The note A4, ecnoded with a note number of 69, is converted
//! to the frequency 440Hz.
double SYMPH_API midi_number_to_frequency(double midi_number);

#ifdef __cplusplus
}
#endif

#endif /* !MIDI_H_ */
