#ifndef PDA_H_
#define PDA_H_

#include "v2p.h"
#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct algorithm_descriptor_boersma;
struct algorithm_descriptor_boersma_unvoiced;
typedef struct algorithm_descriptor_boersma algorithm_descriptor_boersma_t;
typedef struct algorithm_descriptor_boersma_unvoiced algorithm_descriptor_boersma_unvoiced_t;

//! Allocate a boersma structure and initialize it
//! @param frame_size You can pass 0 for default value. Default value is 2048.
//! @param nb_candidates You can pass 0 for default value.
//!                      Default value is 3.
algorithm_descriptor_boersma_t SYMPH_API* boersma_new(uint frame_size, uint nb_candidates);
//! Allocate a boersma unvoiced struct and initialize it
algorithm_descriptor_boersma_unvoiced_t SYMPH_API* boersma_unvoiced_new(uint frame_size);
//! Compute a stream of candidates
//! The number of candidats is in ad->parent.nb_candidates_per_step
//! @return Allocated array of length size_out
candidate_t SYMPH_API* generate_boersma_candidates(
  pitch_analyzer_t* s,
  algorithm_descriptor_boersma_t* ad,
  float* frame_in);
//! Generate the unvoiced candidate
//! The number of candidats is ad->parent.nb_candidates_per_step == 1
//! @return Allocated array of length 1
candidate_t* generate_boersma_unvoiced_candidates(
    pitch_analyzer_t* s,
    algorithm_descriptor_boersma_unvoiced_t* ad,
    float* frame_in);
//! Cut a frame from a stream and an index.
//! Return NULL if more data to the stream are required,
//! or a pointer inside the audio_buffer otherwise.
//! The frame should correspond to the time pointed by the audio_buffer_index.
float* generate_frame_boersma(
    pitch_analyzer_t* s,
    algorithm_descriptor_boersma_t* ad,
    float* audio_buffer,
    unsigned int audio_buffer_index);
//! Transition cost function.
//! It compute the cost of connecting two candidate through the same path.
float boersma_transition_cost(pitch_analyzer_t* s,
  const candidate_t *first, const candidate_t *second);
//! Delete the Algorithm Descriptor
void SYMPH_API boersma_delete(algorithm_descriptor_boersma_t* ad);
//! Delete the Algorithm Descriptor
void SYMPH_API boersma_unvoiced_delete(algorithm_descriptor_boersma_unvoiced_t* ad);

//! Describe the characteristics of a boersma algorithm instance
struct algorithm_descriptor_boersma {
  //! Parent algorithm descriptor (C inheritance)
  struct algorithm_descriptor parent;

  //! Window used for voinced frames
  float* voiced_window;
  //! Autocorrelation of the voiced_window
  float* voiced_window_ac;
  //! Window used for HNR frames
  float* hnr_window;
  //! Autocorrelation of the hnr_window
  float* hnr_window_ac;
};

//! Describe the characteristics of a boersma algorithm instance
struct algorithm_descriptor_boersma_unvoiced {
    //! Parent algorithm descriptor (C inheritance)
    struct algorithm_descriptor parent;
};

#ifdef __cplusplus
}
#endif

#endif /* !PDA_H_ */
