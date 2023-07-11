from .common import *
from .v2p import *
from . import fft

# Resolution of the beat detection algorithm (Number of samples per second)
BEAT_SAMPLING_RATE = 100


# Differentiate a signal
def diff(s):
    return [b - a for a, b in zip(([0]+s)[:-1], ([0]+s)[1:])]


def compute_onset(data):
    import numpy as np
    import scipy
    # Convolve with a 200ms Half Hann Window
    wlen = int(200/1000*44000)
    ha = fft.hann_window(wlen * 2)[wlen:]
    tt = np.concatenate((np.array(ha), np.zeros(len(data)-len(ha))))
    convolved = np.fft.irfft(np.fft.rfft(np.abs(data)) * np.fft.rfft(tt))
    # Down scale to 100Hz
    convolved_down = scipy.signal.resample_poly(convolved, BEAT_SAMPLING_RATE, 44000)
    # Rectification (remove negative part of derivative)
    convolved_r = np.maximum(0, diff(convolved_down))
    return convolved_r


# Compute the energy of a signal normalized by alpha
def compute_energy(T, alpha, y):
    import numpy as np
    return np.sum(np.abs(y) ** 2) * alpha


# Apply comb Filter (T is in samples)
def apply_comb_filter(T, alpha, x, y=[]):
    y = list(y)
    for s in x:
        if len(y) < T:
            last = 0
        else:
            last = y[-T]
        y += [alpha*last + (1-alpha)*s]
    return T, alpha, y


# Comb filter as a lambda T is in samples
def comb_filter(T, alpha):
    return lambda x: apply_comb_filter(T, alpha, x)


def bpm_to_samples(bpm):
    return (60/bpm)*BEAT_SAMPLING_RATE


def samples_to_bpm(smp):
    return 60/(smp/BEAT_SAMPLING_RATE)


# Compute a BANK of lambda functions representing reconfigured comb filters
# t is the half perdiod used to select energy of filters
# A good value is 1500ms
def bank(min_bpm, max_bpm, nb_filters):
    import numpy as np
    #Convert time into samples
    t = 1500/BEAT_SAMPLING_RATE
    max_samples = round(bpm_to_samples(min_bpm))
    min_samples = round(bpm_to_samples(max_bpm))
    filters_rates = np.array(range(min_samples, max_samples + 1))
    b = []
    for T in filters_rates:
        alpha = 0.5 ** (t/T)
        b += [comb_filter(T, alpha)]
    return min_samples, max_samples, b


def compute_resonators(audio, onset_curve=None):
    if onset_curve is None:
        onset_curve = compute_onset(audio)
    mins, maxs, filters = bank(30, 150, 500)
    return mins, maxs, [f(onset_curve) for f in filters]


# Number of samples before next beat
def predict(T, alpha, y):
    import numpy as np
    y = y.copy()
    (T, alpha, y) = apply_comb_filter(T, alpha, np.zeros(T), y)
    return np.argmax(y[-T:])


def compute_prediction(resonator, onset_curve):
    import numpy as np
    (T, alpha, y) = resonator
    # Compute the offset of the filter and feed him silence to align it again
    # with the audio input
    offset = T - (len(onset_curve) % T)
    (T, alpha, y) = apply_comb_filter(T, alpha, np.zeros(offset), y.copy())
    # Predict the first beat from the last forward pass
    size = len(onset_curve)
    predictions = np.zeros(size)
    offset = predict(T, alpha, y)
    # Duplicate this beat at a regular period of T across the recording
    i = 0
    while i + offset < size:
        predictions[i + offset] = 1
        i += T
    return predictions


def compute_prediction_dynamic(resonator, onset_curve):
    import numpy as np
    (T, alpha, y) = resonator
    # Compute the offset of the filter and feed him silence to align it again
    # with the audio input
    offset = T - (len(onset_curve) % T)
    (T, alpha, y) = apply_comb_filter(T, alpha, np.zeros(offset), y.copy())

    # Run comb filter a second time
    w = y.copy()
    (T, alpha, y) = apply_comb_filter(T, alpha, onset_curve, y)

    # Predict for each timesteps from the comb filter
    size = len(onset_curve)
    predictions = np.zeros(size)
    past = []
    for i in range(0, size):
        if i % 2 != 0:
            continue
        # Predict next beat
        idx = predict(T, alpha, y[:i - size])
        if past:
            mean_idx = int(round(np.mean(past)))
            print("mean idx:", mean_idx)
        print("past:", past)
        # If the new beat predicted seams to be a new one and not one reached
        if past and (mean_idx < 1 or abs(mean_idx - idx) > T/1.5):
            if i + mean_idx < size:
                predictions[i + mean_idx] += 1
            past = []
        # Add new index to the past
        if idx > 0:
            past += [idx]
        past = past[-5:]
        past = [*(np.array(past) - 1)]

    return predictions


def detect_tempo(audio):
    import numpy as np
    # Get the offset function
    onset_curve = compute_onset(audio)

    # Compute the best comb filter
    _, _, filtered = compute_resonators(audio, onset_curve=onset_curve)
    best_filter_idx = np.argmax([compute_energy(*z) for z in filtered])
    best_filter = filtered[best_filter_idx]
    (T, alpha, y) = best_filter

    # Get the tempo
    tempo = samples_to_bpm(T)

    # Run the comb filter in predictive mod
    beats = compute_prediction(best_filter, onset_curve)

    return tempo, beats


def detect_tempo_dynamic(audio):
    import numpy as np
    # Get the offset function
    onset_curve = compute_onset(audio)

    # Compute the best comb filter
    _, _, filtered = compute_resonators(audio, onset_curve=onset_curve)
    best_filter_idx = np.argmax([compute_energy(*z) for z in filtered])
    best_filter = filtered[best_filter_idx]
    (T, alpha, y) = best_filter

    # Get the tempo
    tempo = samples_to_bpm(T)

    # Run the comb filter in predictive mod
    beats = compute_prediction_dynamic(best_filter, onset_curve)

    return tempo, beats
