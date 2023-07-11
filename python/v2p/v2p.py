from .common import *

# Type for Candidates (candidate_t)
class CandidateType(ctypes.Structure):
    _fields_ = [
        ("frequency", ctypes.c_float),
        ("amplitude", ctypes.c_float)
    ]

class PitchAnalyzerType(ctypes.Structure):
    _fields_ = [
        ("frame_time_step", ctypes.c_float),
        ("minimal_frequency", ctypes.c_float),
        ("maximal_frequency", ctypes.c_float),
        ("initial_absolute_peak_coeff", ctypes.c_float),
        ("octave_cost", ctypes.c_float),
        ("compute_transition_cost", ctypes.c_void_p),
        ("voiced_unvoiced_cost", ctypes.c_float),
        ("octave_jump_cost", ctypes.c_float),
        ("silence_treshold", ctypes.c_float),
        ("voicing_treshold", ctypes.c_float),
        ("zero_padding", ctypes.c_uint),
        ("minimal_note_length", ctypes.c_uint),
        ("sampling_rate", ctypes.c_float),
        ("delta_t", ctypes.c_float),
        ("candidates", ctypes.POINTER(CandidateType)),
        ("nb_candidates_per_step", ctypes.c_uint),
        ("number_of_timesteps", ctypes.c_uint)
        # Remaining fields not binded
    ]

class AlgorithmDescriptorType(ctypes.Structure):
    _fields_ = [
        ("frame_size", ctypes.c_uint),
        ("nb_candidates_per_step", ctypes.c_uint),
        ("generate_candidates", ctypes.c_void_p),
        ("generate_frame", ctypes.c_void_p),
        ("next", ctypes.c_void_p)
    ]

def ptr_free(ptr):
    handle.v2p_ptr_free.argtypes = [ctypes.c_void_p]
    handle.v2p_ptr_free(ptr)

def v2p_new(timesteps=None):
    if timesteps is None:
        timesteps = 0 # Default timesteps
    handle.v2p_new.argtypes = [ctypes.c_float]
    handle.v2p_new.restype = ctypes.POINTER(PitchAnalyzerType)
    s = handle.v2p_new(timesteps)
    return s;

def v2p_delete(ptr):
    handle.v2p_delete.argtypes = [ctypes.c_void_p]
    handle.v2p_delete(ptr)

def v2p_register_algorithm(s, at):
    handle.v2p_register_algorithm.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    handle.v2p_register_algorithm(s, at);

def __windowed_filter_applicator(filter, data, window_size):
    filter.argtypes = [
        ctypes.POINTER(ctypes.c_float), ctypes.c_uint, ctypes.c_uint
    ]
    filter.restype = ctypes.POINTER(ctypes.c_float)

    # Format input
    length = len(data)
    input = list2cfloats(data)

    # Apply filter and clean memory
    filtered = filter(input, length, window_size)
    out = cfloat_dyn2static(filtered,  length)
    ptr_free(filtered);

    return out

def mean_filter(data, window_size=3):
    return __windowed_filter_applicator(handle.mean_filter, data, window_size)

def median_filter(data, window_size=3):
    return __windowed_filter_applicator(handle.median_filter, data, window_size)
