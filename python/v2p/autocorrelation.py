from .common import *

def autocorrelation(data):
    # Types
    handle.compute_autocorrelation.restype = ctypes.POINTER(ctypes.c_float)
    handle.compute_autocorrelation.argtypes = [
      ctypes.POINTER(ctypes.c_float),
      ctypes.c_uint,
      ctypes.POINTER(ctypes.c_uint)
    ]

    #Create a ctypes array for input
    input = (ctypes.c_float * len(data))(*data)
    size_in = len(data)

    # Compute autocorrelation
    size_out = ctypes.c_uint()
    out_ptr = handle.compute_autocorrelation(input, size_in, ctypes.byref(size_out))
    # Copy it to a fixed size buffer
    out = (ctypes.c_float * size_out.value)()
    ctypes.memmove(out, out_ptr, ctypes.sizeof(ctypes.c_float) * size_out.value)
    # Free unused memory
    handle.v2p_ptr_free(out_ptr)
    return out

def corrected_autocorrelation(data, window):
    # Types
    handle.compute_corrected_autocorrelation.restype = ctypes.POINTER(ctypes.c_float)
    handle.compute_corrected_autocorrelation.argtypes = [
      ctypes.POINTER(ctypes.c_float),
      ctypes.POINTER(ctypes.c_float),
      ctypes.POINTER(ctypes.POINTER(ctypes.c_float)),
      ctypes.c_uint,
      ctypes.POINTER(ctypes.c_uint)
    ]

    #Create a ctypes array for input and window
    frame_in = (ctypes.c_float * len(data))(*data)
    window_in = (ctypes.c_float * len(window))(*window)
    size_in = len(data)

    # Compute autocorrelation
    size_out = ctypes.c_uint()
    out_ptr = handle.compute_corrected_autocorrelation(
      frame_in, window_in, None, size_in, ctypes.byref(size_out)
    )
    # Copy it to a fixed size buffer
    out = (ctypes.c_float * size_out.value)()
    ctypes.memmove(out, out_ptr, ctypes.sizeof(ctypes.c_float) * size_out.value)

    # Free unused memory
    handle.v2p_ptr_free(out_ptr)
    return out
