import ctypes
import os.path

# Handle to the C shared library
handle = None


# Allow to load a dll with ctypes
def load_dll(path):
    global handle
    handle = ctypes.CDLL(path)

module_dir_abs_path = os.path.dirname(os.path.abspath(__file__))
if os.path.isfile(module_dir_abs_path + "/v2p-api.dll"):
    load_dll(module_dir_abs_path + "/v2p-api.dll")


def wav_load(path, sampling_rate=48000):
    from scipy.io import wavfile
    import scipy.signal as sps
    file_sampling_rate, data = wavfile.read(path)
    # Remove stereo
    if len(data.shape) > 1:
        # Take left channel
        data = data.T[0]
    # Resample data if required
    if file_sampling_rate != sampling_rate:
        old_len = len(data)
        new_len = int(old_len * sampling_rate / file_sampling_rate)
        return sps.resample_poly(data, new_len, old_len)
    else:
        return data


# Convert a list into an array of c_float
def list2cfloats(list):
    return (ctypes.c_float * len(list))(*list)


# Convert a dynamicly allocated c_float array to a static array
def ctype_dyn2static(type, dynamic_array, length):
    static_array = (type * length)()
    ctypes.memmove(
        static_array, dynamic_array,
        ctypes.sizeof(type) * length
    )
    return static_array


def cfloat_dyn2static(dynamic_array, length):
    return ctype_dyn2static(ctypes.c_float, dynamic_array, length)
