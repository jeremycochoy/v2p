#ifndef PDA_MAXFREQ_H_
#define PDA_MAXFREQ_H_

#include "v2p.h"
#include "v2p_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct algorithm_descriptor_maxfreq;
typedef struct algorithm_descriptor_maxfreq algorithm_descriptor_maxfreq_t;

//! Allocate a maxfreq struct and initialize it
//! This algorithm is implemented to be run with a boersma instance.
//! The frame_size should be the same as the boersma instance.
algorithm_descriptor_maxfreq_t SYMPH_API* maxfreq_new(uint frame_size);
//! Delete the Algorithm Descriptor
void SYMPH_API maxfreq_delete(algorithm_descriptor_maxfreq_t* ad);

//! Max Frequency observed in the FFT.
//! Rely on boersma implementation which updates `s->last_fft`.
//! Therefore, this algorithm should be added befor a boersma instance.
//! (The insertion order is reversed to the running order)
//!
//! Compute a stream of candidates.
//! The number of candidats is in ad->parent.nb_candidates_per_step
//! @return Allocated array of length size_out
candidate_t SYMPH_API* generate_maxfreq_candidates(
  pitch_analyzer_t*,
  algorithm_descriptor_maxfreq_t* ad,
  float* frame_in);

//! Describe the characteristics of a boersma algorithm instance
struct algorithm_descriptor_maxfreq {
    //! Parent algorithm descriptor (C inheritance)
    struct algorithm_descriptor parent;
};


#ifdef __cplusplus
}
#endif

#endif /* !PDA_MAXFREQ_H_ */
