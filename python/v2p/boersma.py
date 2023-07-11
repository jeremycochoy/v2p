from .common import *
from .v2p import *


def new_boersma(frame_size, nb_candidates = None):
    handle.boersma_new.restype = ctypes.POINTER(AlgorithmDescriptorType)
    handle.boersma_new.argtypes = [ctypes.c_uint]
    at = handle.boersma_new(frame_size)
    if (nb_candidates):
        at[0].nb_candidates_per_step = nb_candidates
    return at;


def new_boersma_unvoiced(frame_size):
    handle.boersma_unvoiced_new.restype = ctypes.POINTER(AlgorithmDescriptorType)
    handle.boersma_unvoiced_new.argtypes = [ctypes.c_uint]
    at = handle.boersma_unvoiced_new(frame_size)
    return at;


def new_maxfreq(frame_size):
    handle.maxfreq_new.restype = ctypes.POINTER(AlgorithmDescriptorType)
    handle.maxfreq_new.argtypes = [ctypes.c_uint]
    at = handle.maxfreq_new(frame_size)
    return at;


def boersma_engine(frame_size=2048, nb_candidates=None, timesteps=None, s=None):
    # Set up the environment
    if s is None:
        s = v2p_new(timesteps)
    algos = []
    # Count the unvoiced frame as a candidate
    if nb_candidates is not None:
        nb_candidates_voiced = nb_candidates - 1
    else:
        nb_candidates_voiced = 3
        nb_candidates = 4
    # Allocate algorithms and connect them
    b_unvoiced = new_boersma_unvoiced(frame_size)
    v2p_register_algorithm(s, b_unvoiced)
    algos.append(b_unvoiced)
    if (nb_candidates_voiced > 0):
        b_voiced = new_boersma(frame_size, nb_candidates)
        v2p_register_algorithm(s, b_voiced)
        algos.append(b_voiced)
    return s, algos


def maxfreq_engine(frame_size=2048, nb_candidates=None, timesteps=None):
    if nb_candidates is not None and nb_candidates > 1:
        nb_candidates -= 1
    s = v2p_new(timesteps)
    # Allocate algorithm and connect it
    f = new_maxfreq(frame_size)
    v2p_register_algorithm(s, f)
    # Add boersma
    s, algos = boersma_engine(frame_size, nb_candidates, timesteps, s=s)
    algos = [s] + algos
    return s, algos


def boersma_candidates(data, frame_size=2048, nb_candidates=None, timesteps=None):
    # Set up the environment
    s, _ = boersma_engine(frame_size=frame_size, nb_candidates=nb_candidates, timesteps=timesteps)

    # Run inline algorithm
    handle.v2p_add_samples.argtypes = [
      ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint
    ]

    input = list2cfloats(data)
    handle.v2p_add_samples(s, input, len(data));

    # Make a copy of s->candidates
    nb =  handle.v2p_nb_candidates_generated(s)
    candidates = ctype_dyn2static(CandidateType, s[0].candidates, nb)

    v2p_delete(s)

    return candidates


def maxfreq_candidates(data, frame_size=2048, nb_candidates=None, timesteps=None):
    # Set up the environment
    s, _ = maxfreq_engine(frame_size=frame_size, nb_candidates=nb_candidates, timesteps=timesteps)

    # Run inline algorithm
    handle.v2p_add_samples.argtypes = [
      ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint
    ]

    input = list2cfloats(data)
    handle.v2p_add_samples(s, input, len(data));

    # Make a copy of s->candidates
    nb =  handle.v2p_nb_candidates_generated(s)
    candidates = ctype_dyn2static(CandidateType, s[0].candidates, nb)

    v2p_delete(s)

    return candidates


def boersma_path(data, frame_size=2048, nb_candidates=None, timesteps=None):
    # Set up the environment
    s, _ = boersma_engine(frame_size=frame_size, nb_candidates=nb_candidates, timesteps=timesteps)

    # Run inline algorithm
    handle.v2p_add_samples.argtypes = [
      ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint
    ]

    input = (ctypes.c_float * len(data))(*data)
    handle.v2p_add_samples(s, input, len(data));

    # Make a copy of path
    handle.v2p_compute_path.argtypes = [ctypes.c_void_p]
    handle.v2p_compute_path.restype = ctypes.POINTER(ctypes.c_float)
    path = handle.v2p_compute_path(s);
    path_copy = cfloat_dyn2static(path, s[0].number_of_timesteps)
    ptr_free(path);

    # Free engine
    # Todo : Free boersma
    v2p_delete(s)

    return path_copy


def maxfreq_path(data, frame_size=2048, nb_candidates=None, timesteps=None):
    # Set up the environment
    s, _ = maxfreq_engine(frame_size=frame_size, nb_candidates=nb_candidates, timesteps=timesteps)

    # Run inline algorithm
    handle.v2p_add_samples.argtypes = [
      ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint
    ]

    input = (ctypes.c_float * len(data))(*data)
    handle.v2p_add_samples(s, input, len(data))

    # Make a copy of path
    handle.v2p_compute_path.argtypes = [ctypes.c_void_p]
    handle.v2p_compute_path.restype = ctypes.POINTER(ctypes.c_float)
    path = handle.v2p_compute_path(s)
    path_copy = cfloat_dyn2static(path, s[0].number_of_timesteps)
    ptr_free(path);

    # Free engine
    # Todo : Free boersma
    v2p_delete(s)

    return path_copy


def v2p_path(data, frame_size=2048, nb_candidates=None, timesteps=None):
    # Set up the environment
    s, _ = boersma_engine(frame_size=frame_size, nb_candidates=nb_candidates, timesteps=timesteps)

    # Run inline algorithm
    handle.v2p_add_samples.argtypes = [
      ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint
    ]

    input = (ctypes.c_float * len(data))(*data)
    handle.v2p_add_samples(s, input, len(data));

    # Make a copy of path
    handle.v2p_compute_path.argtypes = [ctypes.c_void_p]
    handle.v2p_compute_path.restype = ctypes.POINTER(ctypes.c_float)
    path = handle.v2p_compute_path(s);
    path_copy = cfloat_dyn2static(path, s[0].number_of_timesteps)
    ptr_free(path);

    # Free engine
    # Todo : Free boersma
    v2p_delete(s)

    return path_copy
