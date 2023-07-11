import v2p as s
import matplotlib.pyplot as plt
import math
import random
import numpy as np


def show_windows(size = 4096):
    figure = plt.figure()
    plt.title("Windows defined by FFT module")
    plt.plot(s.hann_window(size), label = "hann")
    plt.plot(s.hamming_window(size), label = "hamming")
    plt.plot(s.blackman_harris_window(size), label = "blackman_harris")
    plt.legend(loc='upper left')
    plt.show()


def show_autocorrelation(size = 300):
    figure = plt.figure()
    plt.title("Autocorrelation of some functions")
    a = [math.sin(0.1*x) for x in range(size)]
    b = [math.cos(0.05*x) * 1.5 * random.random() for x in range(size)]
    x = s.autocorrelation(a)
    y = s.autocorrelation(b)
    plt.subplot(211)
    plt.plot(a, label = "sinus")
    plt.plot(b, label = "random cos")
    plt.legend(loc='upper left')
    plt.subplot(212)
    plt.plot(x, label = "ac: sinus")
    plt.plot(y, label = "ac: random cos")
    plt.legend(loc='upper left')
    plt.show()


def show_corrected_autocorrelation(size = 1000):
    figure = plt.figure()
    plt.title("Corrected Autocorrelation")

    ax1 = plt.subplot(221)
    ax1.set_title("Functions")
    a = [math.sin(0.02*x) for x in range(size)]
    b = [
      math.sin(x * 0.025) * 0.7 +
      math.cos(x * 0.04) * 0.6 +
      math.sin(x * 0.05 + math.pi / 3) * 0.4
      for x in range(size)
    ]
    a_name = "sinus"
    b_name = "multiple freqs"
    plt.plot(a, label = a_name)
    plt.plot(b, label = b_name)
    plt.legend(loc='lower center')

    ax2 = plt.subplot(222)
    ax2.set_title("AC with Hann window")
    x = s.corrected_autocorrelation(a, s.hann_window(size))
    y = s.corrected_autocorrelation(b, s.hann_window(size))
    plt.plot(x, label = "ac: " + a_name)
    plt.plot(y, label = "ac: " + b_name)
    plt.legend(loc='lower center')
    plt.subplot(221)

    ax3 = plt.subplot(223, sharey=ax2)
    ax3.set_title("AC with Hamming window")
    x = s.corrected_autocorrelation(a, s.hamming_window(size))
    y = s.corrected_autocorrelation(b, s.hamming_window(size))
    plt.plot(x, label = "ac: " + a_name)
    plt.plot(y, label = "ac: " + b_name)
    plt.legend(loc='lower center')

    ax4 = plt.subplot(224, sharey=ax3)
    ax4.set_title("AC with BlackmanHarris window")
    x = s.corrected_autocorrelation(a, s.blackman_harris_window(size))
    y = s.corrected_autocorrelation(b, s.blackman_harris_window(size))
    plt.plot(x, label = "ac: " + a_name)
    plt.plot(y, label = "ac: " + b_name)
    plt.legend(loc='lower center')

    plt.show()


def boersma_get_xyc(input, nb_candidates=4, timesteps=None):
    r = s.boersma_candidates(input, nb_candidates=nb_candidates, timesteps=timesteps)
    y = s.pitch_to_midi_numbers([x.frequency for x in r])
    x = [int(x / nb_candidates) for x in range(len(y))]
    c = [x.amplitude for x in r]
    return (x, y, c)


def show_boersma_candidates(signal=None, timesteps=None, frame_size=None, nb_candidates=4, keys=False):
    if signal is None:
        signal = signal_128hz()
    x, y, c = boersma_get_xyc(signal, nb_candidates=nb_candidates, timesteps=timesteps)

    if keys:
        show_keys(max(x))
    s = plt.scatter(x, y, c=c)
    plt.colorbar(s)
    plt.show()

    return (x, y, c)


def maxfreq_get_xyc(input, nb_candidates=4, timesteps=None):
    r = s.maxfreq_candidates(input, nb_candidates=nb_candidates, timesteps=timesteps)
    y = s.pitch_to_midi_numbers([x.frequency for x in r])
    x = [int(x / nb_candidates) for x in range(len(y))]
    c = [x.amplitude for x in r]
    return (x, y, c)


def show_maxfreq_candidates(signal=None, timesteps=None, frame_size=None, nb_candidates=4, keys=False):
    if signal is None:
        signal = signal_128hz()
    x, y, c = maxfreq_get_xyc(signal, nb_candidates=nb_candidates, timesteps=timesteps)

    if keys:
        show_keys(max(x))
    s = plt.scatter(x, y, c=c)
    plt.colorbar(s)
    plt.show()

    return (x, y, c)


def show_keys(len, target=plt):
    keys = [33, 45, 57, 69]
    for a_number in keys:
        for k in range(0, 12):
            number = a_number + k
            if (k == 0):
                target.plot([0, len], [number, number], c='black')
            else:
                target.plot([0, len], [number, number], c='grey')


def show_boersma_path(signal=None, frame_size=None, timesteps=None, nb_candidates=3, keys=False):
    if signal is None:
        signal = signal_150hz_incresing()
    r = s.boersma_path(signal, nb_candidates = nb_candidates, timesteps=timesteps);

    if keys:
        show_keys(len(r))
    plt.plot(r, 'b-');
    plt.plot(r, 'r+');

    plt.show()

    return r


def show_filtered_paths(signal=None, frame_size=None, timesteps=None, nb_candidates=3, keys=False, window_size=3):
    if signal is None:
        signal = signal_150hz_incresing()
    r = s.v2p_boersma_path(signal, nb_candidates = nb_candidates, timesteps=timesteps);
    r_mu = s.mean_filter(r, window_size=window_size)
    r_nu = s.median_filter(r, window_size=window_size)
    r_numu = s.mean_filter(r_nu, window_size=window_size)

    if keys:
        show_keys(len(r))

    plt.plot(r_mu, '+', label = "mean")
    plt.plot(r_nu, '+', label = "median")
    plt.plot(r_numu, '+', label = "mean . median")
    plt.plot(r, '+', label = "raw")
    plt.legend(loc='lower center')

    plt.show()
    return (r, r_mu, r_nu, r_numu)


def signal_128hz(len=48000):
    return [math.sin(x * 128 * 2 * 3.14 / 48000) for x in range(len)]


def signal_150hz_incresing(len=48000):
    return [math.sin(x * 2 * 3.14 * (150 + (x/1000.0)) / 48000) for x in range(len)]

def show_midi_distances(pitch=range(440,660), interval=(-1, 1), steps=100):

    numbers = s.pitch_to_midi_numbers(pitch)
    interval = np.linspace(*interval, steps)
    distances_signed = []
    distances_abs = []
    distance_noteweigth = []
    distance_noteweigth_abs = []

    # For each value mu of correction in the input interval
    for mu in interval:
        # Translate midi numbers
        numbers_corrected = [x + mu if x > 0 else 0 for x in numbers]
        # Compute the distance using note segmentation
        midi_notes = s.midi_numbers_to_notes(numbers_corrected)

        curve_distance = [s.distance_to_grid(v.note_number) * v.duration for v in midi_notes]
        note_distance = [s.distance_to_grid(v.note_number) for v in midi_notes]

        d = sum(curve_distance) / np.sum([v.duration for v in midi_notes])
        d_abs = sum(map(abs, curve_distance)) / np.sum([v.duration for v in midi_notes])
        nd = sum(note_distance) / len(note_distance)
        nd_abs = sum(map(abs, note_distance)) / len(note_distance)

        distances_signed += [d]
        distances_abs += [d_abs]
        distance_noteweigth += [nd]
        distance_noteweigth_abs += [nd_abs]

    plt.plot(interval, distances_abs, label="distance")
    plt.plot(interval, distances_signed, label="signed distance")
    plt.plot(interval, distance_noteweigth_abs, label="distance by note")
    plt.plot(interval, distance_noteweigth, label="signed distance by note")
    plt.legend(loc='lower left')
    plt.xlabel("Half tone translation")

    plt.show()


def show_midi_segmentation(pitch):
    # Compute paths
    numbers = s.pitch_to_midi_numbers(pitch)
    numbers_snaped = np.round(numbers)

    #Compute notes (key note pressed by time unit) array
    r = s.midi_numbers_to_notes(numbers)
    curve = np.array(0.0).repeat(len(pitch)) # silence (note = 0)
    for note in r:
        curve[int(note.position):int(note.position + note.duration)] = np.array(note.note_number).repeat(int(note.duration))

    # Create color table
    c_tab = ["red", "green", "blue", "yellow"]
    colors = ["black" for x in range(len(pitch))] # Black for untouched points
    for i, note in enumerate(r):
        colors[int(note.position):int(note.position) + int(note.duration)] = np.array([c_tab[i % 4]]).repeat(int(note.duration))

    # Plot the midi keyboard
    p1 = plt.subplot(221)
    x = np.array(range(0, len(curve)))
    y = curve
    plt.scatter(x, y, c=colors)

    # Plot the distances to grid
    p2 = plt.subplot(222, sharex=p1)
    d = [s.distance_to_grid(v) for v in numbers]
    u = [abs(v) for v in d]
    plt.plot(d, label="signed distance")
    plt.plot(u, label="distance")
    plt.legend(loc='lower center')

    # Plot the pitch in log2 base and colored by segments
    p3 = plt.subplot(223, sharex=p1)
    x = np.array(range(0, len(numbers)))
    y = numbers
    plt.scatter(x, y, c=colors)

    # Plot the pitch in log2 base and colored by segments
    p4 = plt.subplot(224, sharex=p1, sharey=p3)
    x = np.array(range(0, len(numbers_snaped)))
    y = numbers_snaped
    plt.scatter(x, y, c=colors)
    plt.show()

    return (r, curve, numbers_snaped)


def show_tempo_response(audio=None, resonators=None):
    if resonators is None:
        resonators = s.compute_resonators(audio)
    min_samples, max_samples, filtered = resonators
    # Energy of filters
    x = [s.samples_to_bpm(v) for v in range(min_samples, max_samples + 1)]
    y1 = [s.compute_energy(*z) for z in filtered]
    plt.plot(x, y1)
    # Energy of audio
    if audio is not None:
        y2 = [np.sum(np.abs(x) ** 2) * alpha for (T, alpha, y) in filtered]
        plt.plot(x, y2)
    plt.show()


def show_tempo_segmentation(audio):
    # Plot Onset Curve
    onset_curve = s.compute_onset(audio)
    x = np.linspace(0, 1, len(onset_curve))
    plt.plot(x, 80 * onset_curve / np.max(onset_curve))

    # Plot the tempo predictions
    tempo, beats = s.detect_tempo(audio)
    plt.plot(np.linspace(0, 1, len(beats)), 80 * beats / np.max(beats))

    # Compute notes
    numbers = s.pitch_to_midi_numbers(s.boersma_path(audio))
    notes = s.midi_numbers_to_notes(numbers)
    curve = np.zeros(len(numbers)) - 1
    for note in notes:
        curve[int(note.position):int(note.position + note.duration)] = np.array(note.note_number).repeat(int(note.duration))

    # Create color table
    c_tab = ["red", "green", "blue", "yellow"]
    # Black for untouched points
    colors = ["black" for _ in range(len(numbers))]
    for i, note in enumerate(notes):
        colors[int(note.position):int(note.position) + int(note.duration)] = np.array([c_tab[i % 4]]).repeat(int(note.duration))

    # Plot the notes
    x = np.linspace(0, 1, len(curve))
    plt.scatter(x, curve, c=colors)

    plt.show()


def show_tempo_segmentation_dynamic(audio):
    # Plot Onset Curve
    onset_curve = s.compute_onset(audio)
    x = np.linspace(0, 1, len(onset_curve))
    plt.plot(x, 80 * onset_curve / np.max(onset_curve))

    # Plot the tempo predictions
    tempo, beats = s.detect_tempo_dynamic(audio)
    plt.plot(np.linspace(0, 1, len(beats)), 80 * beats / np.max(beats))

    # Compute notes
    numbers = s.pitch_to_midi_numbers(s.boersma_path(audio))
    notes = s.midi_numbers_to_notes(numbers)
    curve = np.zeros(len(numbers)) - 1
    for note in notes:
        curve[int(note.position):int(note.position + note.duration)] = np.array(note.note_number).repeat(int(note.duration))

    # Create color table
    c_tab = ["red", "green", "blue", "yellow"]
    # Black for untouched points
    colors = ["black" for _ in range(len(numbers))]
    for i, note in enumerate(notes):
        colors[int(note.position):int(note.position) + int(note.duration)] = np.array([c_tab[i % 4]]).repeat(int(note.duration))

    # Plot the notes
    x = np.linspace(0, 1, len(curve))
    plt.scatter(x, curve, c=colors)

    plt.show()
