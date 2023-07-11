#ifndef V2P_API_H_
#define V2P_API_H_

#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Forward declations and typedefs
//
struct algorithm_descriptor;
struct pitch_analyzer;
struct candidate;
typedef struct algorithm_descriptor algorithm_descriptor_t;
typedef struct pitch_analyzer pitch_analyzer_t;
typedef struct candidate candidate_t;
typedef unsigned int uint;
//! Stretchy buffer float array
//! We use them to remember that the memory was allocated by a stretchy buffer.
typedef float* sb_float;
//! Stretchy buffer uint array
//! We use them to remember that the memory was allocated by a stretchy buffer.
typedef uint* sb_uint;
//! Stretchy buffer candidate_t array
//! We use them to remember that the memory was allocated by a stretchy buffer.
typedef candidate_t* sb_candidate;

//! Type of the candidate geratore calling an algorithm implementation
typedef candidate_t* (*algorithm_t)(struct pitch_analyzer*, struct algorithm_descriptor*, float*);
//! Type of the function cuting frames from the buffer
typedef float* (*framer_t)(struct pitch_analyzer*, struct algorithm_descriptor*,
  float* buffer, unsigned int buffer_index);
//! Type of the function used to compute the cost for a transition
//! between two candidates.
typedef float (*coster_t)(struct pitch_analyzer*, candidate_t *first, candidate_t *second);

//
// Functions which can be called to use the api
//

//! Allocate a sylmphonia object
//! @param timesteps Set it to 0 for default timesteps value (10ms).
struct pitch_analyzer SYMPH_API* v2p_new(float timesteps);
//! Free memory and internal substructures
void SYMPH_API v2p_delete(pitch_analyzer_t* s);
//! Compute internal value for the next run. This function should be called
//! if you changed any of the values stored into s.
void SYMPH_API v2p_reset(pitch_analyzer_t* s);
//! Compute the pitch analyzer online with new samples
void SYMPH_API v2p_add_samples(pitch_analyzer_t* s, const float* samples_in, unsigned int size_in);
//! Compute the pitch analyzer on the whole audio_buffer (TODO)
void v2p_run(pitch_analyzer_t* s, float* audio_buffer, unsigned int size);
//! Compute the resulting pitch path. A 0 frequency means a silence.
float SYMPH_API* v2p_compute_path(pitch_analyzer_t*);
//! Return the total number of samples in a computed path.
unsigned int SYMPH_API v2p_path_len(pitch_analyzer_t*);
//! Insert the algorithm described by algorithm_descriptor
//! into the list of algorithms stored by pitch_analyser.
//!
//! Last inserted algorithm will be the first executed ;
//! execution occurs in reverse inserting order.
//!
//! Once an algorithm is registered, it cannot be undone.
void SYMPH_API v2p_register_algorithm(pitch_analyzer_t*, struct algorithm_descriptor*);
//! Return the total number of candidates computed
unsigned int SYMPH_API v2p_nb_candidates_generated(pitch_analyzer_t*);
//! The free associated to the malloc used by the library
void SYMPH_API* v2p_ptr_free(void* ptr);

//! Post process the pitch path with the median of the given window size.
//! It keeps silences unchanged.
//! @return An new allocated array
float SYMPH_API* median_filter(float* buffer, uint length, uint window_size);

//! Post process the pitch path with the mean of the given window size,
//! ignoring zero values.
//! It keeps silences unchanged. (TODO)
//! @return An new allocated array
float SYMPH_API* mean_filter(float* buffer, uint length, uint window_size);

//! Structure containing the internal settings of the v2p engine
struct pitch_analyzer {

  // Settings : They can be changed right affter v2p_init,
  //            but you have to call v2p_reset before usingt the
  //            pitch_analyzer.

  //! Time step between two frames
  float frame_time_step;
  //! Minimal frequency recorded
  float minimal_frequency;
  //! Maximal frequency recorded
  float maximal_frequency;
  //! Multiplicator to apply to the first
  //! global_absolute_peak mesure. Put it
  //! to 1.0f for no effect, or > 1.0f
  //! for considering that the first sample is silence.
  float initial_absolute_peak_coeff;
  //! octave cost (bias which favorite higher frequencies)
  float octave_cost;
  //! Function used to compute the transition cost between two
  //! candidates.
  coster_t compute_transition_cost;
  //! Transition cost from voiced to unvoied / unvoiced to voiced
  float voiced_unvoiced_cost;
  //! Cost for a transition between two frequencies
  float octave_jump_cost;
  //! Threshold to control detection of voiced/unvoiced.
  float silence_threshold;
  //! Threshold to control detection of voiced/unvoiced.
  float voicing_threshold;
  //! length of the zero padding
  unsigned int zero_padding;
  //! Note of length strictly smaller than this value will be removed.
  //! The unit is in samples.
  unsigned int minimal_note_length;

  //
  // Informations used for computation
  //

  //! Sampling frequency of the audio stream
  float sampling_rate;
  //! Time interval in seconds between two samples (1.f/sampling_frequency)
  float delta_t;
  //! Candidates build by running the algorithms
  //! candidate_t[number_of_timesteps][nb_candidates_per_step]
  candidate_t* candidates;
  //! Total number of candidates which will be computed
  //! by the different algorithms.
  unsigned int nb_candidates_per_step;
  //! Number of step browsed during the
  unsigned int number_of_timesteps;

  //! Can be used freely by the user.
  //! This field is ignored by V2p.
  void* user_data;

  //
  // Internal machinery
  //

  //! Buffer of audio samples
  sb_float audio_buffer;
  //! Index for the current online processing off the buffer
  unsigned int audio_buffer_index;
  //! Size of a step inside the audio_buffer
  unsigned int frame_step_size;
  //! Amplitude maximal detected through the stream
  float global_absolute_peak;
  //! Algorithms used to generate candidates
  struct algorithm_descriptor* algorithm_descriptors;
  //! List of costs through paths (viterbi)
  float* path_costs;
  //! List of indexes through path (viterbi)
  sb_uint path_indexes;
  //! Last fft computed and stored by boersma algorithm (allocated)
  float* last_fft;
  unsigned int last_fft_size;
};

//! An algorithm receive its configuration and the index of the next available line
//! in the candidate_table.
struct algorithm_descriptor {
  //! Size of a frame
  unsigned int frame_size;
  //! Number of candidates generated
  unsigned int nb_candidates_per_step;
  //! Function used to generate a new candidate
  algorithm_t generate_candidates;
  //! Function used to generate a new frame
  framer_t generate_frame;

  struct algorithm_descriptor* next;
};

//! Structure containing a paire frequency/amplitude.
//! Amplitude can be either the amplitude of a signal or the weight
//! of the associated frequency.
//! Unvoiced frames are described as a frequency of 0.
struct candidate {
  float frequency;
  union {
    float amplitude;
    float weight;
  };
};

//
// Internal implementation
//

//! Compute Viterbi path finding algorithm O(nb_candidates^2 * path_length)
//! Its a dynamic programic algorithm from Viterbi which allows to
//! find the best path through candidates.
void update_viterbi_path(pitch_analyzer_t* s);

#ifdef __cplusplus
}
#endif

#endif /* !V2P_API_H_ */
