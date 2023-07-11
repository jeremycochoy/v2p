from .common import *
from .v2p import *
from .boersma import *


# Type for Candidates (candidate_t)
class MidiNoteType(ctypes.Structure):
    _fields_ = [
        ("note_number", ctypes.c_float),
        ("velocity", ctypes.c_uint),
        ("duration", ctypes.c_float),
        ("position", ctypes.c_float)
    ]

    def to_dic(self):
        return {
            "note_number": self.note_number,
            "velocity": self.velocity,
            "duration": self.duration,
            "position": self.position,
        }

    def __repr__(self):
        addr = super(ctypes.Structure, self).__repr__()
        dic = self.to_dic()
        return addr[:-1] + " " + str(dic) + ">"

def frequency_to_midi_number(freq):
    handle.frequency_to_midi_number.argtypes = [ctypes.c_double]
    handle.frequency_to_midi_number.restype = ctypes.c_double
    return handle.frequency_to_midi_number(ctypes.c_double(freq))

def midi_number_to_frequency(midi_number):
    handle.midi_number_to_frequency.argtypes = [ctypes.c_double]
    handle.midi_number_to_frequency.restype = ctypes.c_double
    return handle.midi_number_to_frequency(ctypes.c_double(midi_number))

def distance_to_grid(midi_number):
    handle.distance_to_grid.argtypes = [ctypes.c_float]
    handle.distance_to_grid.restype = ctypes.c_float
    return handle.distance_to_grid(ctypes.c_float(midi_number))

def pitch_to_midi_numbers(pitch):
    handle.pitch_to_midi_numbers.argtypes = [
        ctypes.POINTER(ctypes.c_float),
        ctypes.c_uint
    ]
    handle.pitch_to_midi_numbers.restype = ctypes.POINTER(ctypes.c_float)

    # Format input
    length = len(pitch)
    input = list2cfloats(pitch)

    # Convert and clean memory
    numbers = handle.pitch_to_midi_numbers(input, length)
    numbers_copy = cfloat_dyn2static(numbers, length)
    ptr_free(numbers)

    return numbers_copy


def midi_numbers_to_notes(midi_numbers):
    handle.midi_numbers_to_notes.argtypes = [
        ctypes.POINTER(ctypes.c_float),
        ctypes.c_uint,
        ctypes.POINTER(ctypes.c_uint)
    ]
    handle.midi_numbers_to_notes.restype = ctypes.POINTER(MidiNoteType)

    # Format input
    length = len(midi_numbers)
    input = list2cfloats(midi_numbers)
    out_size = ctypes.c_uint()

    # Apply filter and clean memory
    notes = handle.midi_numbers_to_notes(input, length, ctypes.byref(out_size))
    out = ctype_dyn2static(MidiNoteType, notes, out_size.value)
    ptr_free(notes)

    # TODO : Implement this in C
    notes = [n for n in out if n.note_number > 0]
    out = (MidiNoteType * len(notes))(*notes)

    return out


def pitch_to_midi(pitch, filename="sample.mid"):
    return numbers_to_midi(pitch_to_midi_numbers(pitch), filename)


def notes_to_midi(notes, file_name="sample.mid"):
    from mido import Message, MetaMessage, MidiFile, MidiTrack

    # Create midi events
    sss = []
    last_end = 0
    for x in notes:
        sss += [
            Message('note_on', note=round(x.note_number), velocity=x.velocity, time=int(x.position-last_end))
        ]
        sss += [
            Message('note_off', note=round(x.note_number), velocity=x.velocity, time=int(x.duration))
        ]
        last_end = x.position + x.duration

    # Write file (120BPM means 0.5s per beat so since a tick is 0.01s, we have a tpb of 50)
    mid = MidiFile(ticks_per_beat=50)
    track = MidiTrack()
    mid.tracks.append(track)

    for x in sss:
        track.append(x)

    mid.save(file_name)
    return mid, track, file_name


def numbers_to_midi(numbers, file_name="sample.mid"):
    # Create midi events
    notes = midi_numbers_to_notes(numbers)
    return notes_to_midi(notes)


def correct_numbers_grid(numbers):
    """
    Translate all midi numbers together to minimise the sum of distances
    to the grid.

    :param numbers: Midi numbers to be affected
    :return:
    """
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) and select the one minimizing the grid distance.

    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    for mu in interval:
        # Translate midi numbers
        translated_numbers = [x + mu if x > 0.5 else 0 for x in numbers]
        # Compute distance to the grid
        note_distances = map(distance_to_grid, translated_numbers)
        distances += [sum(map(abs, note_distances))]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return np.array([x + translation if x > 0.5 else 0 for x in numbers])


def correct_numbers_grid_using_mean_notes(numbers):
    """
    Translate all midi numbers together to minimise the sum of distances
    of the mean note value to the grid.

    :param numbers: Midi numbers to be affected
    :return:
    """
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) and select the one minimizing the grid distance.

    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    for mu in interval:
        # Translate midi numbers
        translated_numbers = [x + mu if x > 0.5 else 0 for x in numbers]
        # Split midi numbers into notes
        notes = midi_numbers_to_notes(translated_numbers)

        # Compute distance to the grid
        def compute_note_mean(note):
            note_samples = translated_numbers[int(note.position):int(note.position + note.duration)]
            length = len(note_samples) if len(note_samples) > 0 else 1
            note_mean = sum(note_samples) / length
            return note_mean

        notes_mean = [compute_note_mean(note) for note in notes]
        notes_mean = [x for x in notes_mean if x > 0.5]
        note_distances = map(distance_to_grid, notes_mean)
        distances += [sum(map(abs, note_distances))]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return np.array([x + translation if x > 0.5 else 0 for x in numbers])


def correct_numbers_grid_using_mean_notes_weighted(numbers):
    """
    Translate all midi numbers together to minimise the sum of distances
    of the mean note value to the grid, where each note is weighted
    by its duration.

    :param numbers: Midi numbers to be affected
    :return:
    """
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) and select the one minimizing the grid distance.

    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    for mu in interval:
        # Translate midi numbers
        translated_numbers = [x + mu if x > 0.5 else 0 for x in numbers]
        # Split midi numbers into notes
        notes = midi_numbers_to_notes(translated_numbers)

        # Compute distance to the grid
        def compute_note_mean(note):
            note_samples = translated_numbers[int(note.position):int(note.position + note.duration)]
            length = len(note_samples) if len(note_samples) > 0 else 1
            note_mean = sum(note_samples) / length
            return note_mean, note.duration

        notes_mean = [compute_note_mean(note) for note in notes]
        notes_mean = [x for x in notes_mean if x[0] > 0.5]
        note_distances = (distance_to_grid(n[0]) * n[1] for n in notes_mean)
        total_weights = sum([n[1] for n in notes_mean])
        distances += [sum(map(abs, note_distances)) / total_weights]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return np.array([x + translation if x > 0.5 else 0 for x in numbers])


def correct_numbers_grid_notes(numbers):
    """
    Outdated implementation (befor buckets)

    :param numbers:
    :return:
    """
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) ans select the one minimizing the grid distance
    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    for mu in interval:
        # Translate midi numbers and compute notes
        notes = midi_numbers_to_notes([x + mu if x > 0.5 else 0 for x in numbers])
        # Compute distance to the grid
        note_distances = [distance_to_grid(n.note_number) for n in notes]
        distances += [sum(map(abs, note_distances)) / len(note_distances)]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return np.array([x + translation if x > 0.5 else 0 for x in numbers])


def correct_numbers_grid_pondered_notes(numbers):
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) ans select the one minimizing the grid distance
    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    for mu in interval:
        # Translate midi numbers and compute notes
        notes = midi_numbers_to_notes([x + mu if x > 0 else 0 for x in numbers])
        # Compute distance to the grid
        note_distances = [distance_to_grid(n.note_number) * n.duration for n in notes]
        distances += [sum(map(abs, note_distances)) / np.sum([n.duration for n in notes])]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return numbers + translation


def correct_notes_grid(notes):
    import numpy as np
    # Check divers translation of the midi numbers in interval
    # (-0.499, 0.499) ans select the one minimizing the grid distance
    interval = np.linspace(-0.499, 0.499, 100)
    distances = []
    notes_list = []
    for mu in interval:
        # Copy
        notes_cpy = type(notes)(*notes)
        # Translate midi numbers and compute notes
        for n in notes_cpy:
            n.note_number += mu if n.note_number >= 1 else 0
        # Compute distance to the grid
        notes_list += [notes_cpy]
        note_distances = [distance_to_grid(n.note_number) * n.duration for n in notes_cpy]
        distances += [sum(map(abs, note_distances)) / np.sum([n.duration for n in notes_cpy])]
    # Select the translation minimizing the distance to the grid
    translation = interval[np.argmin(distances)]
    return notes_list[np.argmin(distances)]


def correct_notes_intervals(notes):
    import numpy as np
    notes = type(notes)(*notes)
    notes_number = np.array([x.note_number for x in notes])
    intervals = notes_number[1:] - notes_number[:-1]
    intervals_deviation = [x - round(x) for x in intervals]
    # Apply the correction to the following notes
    # Let start unchanged
    for i, corr in enumerate(intervals_deviation):
        # Correction between notes[i] and notes[i+1]
        # Todo: balanced version
        note = notes[i+1]
        note.note_number -= corr
    return notes


def correct_notes_intervals_balanced(notes):
    import numpy as np
    notes = type(notes)(*notes)
    notes_number = np.array([x.note_number for x in notes])
    intervals = notes_number[1:] - notes_number[:-1]
    intervals_deviation = [x - round(x) for x in intervals]
    # Apply the correction to the following notes
    # Let start unchanged
    for i, corr in enumerate(intervals_deviation):
        # Correction between notes[i] and notes[i+1]
        note = notes[i+1]
        l_corr = corr
        r_corr = intervals_deviation[i+1] if i + 1 < len(intervals_deviation) else corr
        note.note_number -= (l_corr + r_corr) / 2
    return notes


def correct_notes_intervals_harsh_propagate(notes):
    notes = type(notes)(*notes)
    for a, b in zip(notes[:-1], notes[1:]):
        interval = b.note_number - a.note_number
        defect = interval - round(interval)
        b.note_number -= defect
    return notes


def correct_notes_intervals_harsh(notes):
    import numpy as np
    notes = type(notes)(*notes)
    notes_number = np.array([x.note_number for x in notes])
    intervals = notes_number[1:] - notes_number[:-1]
    intervals = [x for x in intervals]
    for i, interval in enumerate(intervals):
        print("Interval", i, "has width", interval, "rounded to", round(interval))
        notes[i+1].note_number = notes[i].note_number + round(interval)
    return notes
