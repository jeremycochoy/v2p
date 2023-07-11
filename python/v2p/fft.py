from .common import *

def hann_window(size):
    arr = (ctypes.c_float * size)()
    handle.compute_hann(arr, size)
    return arr

def hamming_window(size):
    arr = (ctypes.c_float * size)()
    handle.compute_hamming(arr, size)
    return arr

def blackman_harris_window(size):
    arr = (ctypes.c_float * size)()
    handle.compute_blackman_harris(arr, size)
    return arr

# Todo: realft et dfft bindings
