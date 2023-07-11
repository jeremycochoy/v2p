import sys
import os
import numpy as np
from math import ceil
# Load the v2p binding
from importlib.machinery import SourceFileLoader
SourceFileLoader("v2p", "./python/v2p/__init__.py").load_module()
from v2p import *


def parse_directory(dirname):
    dic = {}
    for root, directories, files in os.walk(dirname):
        for file in files:
            if '.mns' == file[-4:]:
                path = os.path.join(root, file)
                fileid = path[:-4]
                dic[fileid] = {"mns": mns_load(path)}
                wav_file = path[:-4] + ".wav"
                if os.path.isfile(wav_file):
                    dic[fileid]["wav_file"] = wav_file
    return dic


def mns_load(filename):
    notes = []
    with open(filename) as file:
        for line in file:
            number, start_seconds, end_seconds = [float(s.strip()) for s in line.split(':')]
            note = MidiNoteType()
            note.note_number = number
            note.position = start_seconds * 100 # Unit is 10ms
            note.duration = (end_seconds - start_seconds) * 100 # Unit is 10ms
            notes += [note]
    return notes


def midi_notes_to_numbers(notes):
    max_len = int(ceil(max([n.position+n.duration for n in notes])))
    arr = np.zeros(max_len, dtype='int')
    for n in notes:
        position = int(round(n.position))
        duration = int(round(n.position+n.duration))
        arr[position:position+duration] = int(round(n.note_number))
    return arr


def compute_midi_numbers_distance(notes_a, notes_b):
    curve_a = midi_notes_to_numbers(notes_a)
    curve_b = midi_notes_to_numbers(notes_b)

    # Padding
    max_len = max(len(curve_a), len(curve_b))
    curve_a = np.pad(curve_a, (0, max_len - len(curve_a)), mode='constant')
    curve_b = np.pad(curve_b, (0, max_len - len(curve_b)), mode='constant')
    assert(max_len == len(curve_a))
    assert(max_len == len(curve_b))

    nb_non_zero = len([(a, b) for a, b in zip(curve_a, curve_b) if a is not 0 or b is not 0])
    # Count the proportion of pitch mismatch for non zero numbers
    return np.sum([0 if a == b else 1 for a, b in zip(curve_a, curve_b)]) / nb_non_zero


def compute_accuracy(dataset, fct):
    out = {}
    for fileid, data in dataset.items():
        if "wav_file" not in data:
            print(fileid, " do not have associated wav source. Skipped.")
            continue

        # Load and compute notes from wav file
        print("Load", data["wav_file"], "...")
        wav = wav_load(data["wav_file"])
        notes = fct(wav)

        out[fileid] = {
            "note_count_input": len(notes),
            "note_count_output": len(data["mns"]),
            "note_count_diff": abs(len(data["mns"]) - len(notes)),
            "ghost_notes": max(0, len(notes) - len(data["mns"])),
            "midi_number_distance": compute_midi_numbers_distance(notes, data["mns"]),
            "note_segmentation_distance": abs(len(data["mns"]) - len(notes)) / max(len(notes), len(data["mns"]))
        }
    return out


def v2p_pipeline(audio):
    #pitch = boersma_path(audio)
    pitch = maxfreq_path(audio)
    numbers = pitch_to_midi_numbers(pitch)
    numbers = median_filter(numbers, window_size=3)
    notes = midi_numbers_to_notes(numbers)
    notes_corrected = correct_notes_grid(notes)
    return notes


# Run tests
if __name__ == "__main__":
    print("Compute V2P Pitch accuracy...")
    if len(sys.argv) < 2:
        print("./accuracy.py V2P_SAMPLES_DIRECTORY")
        exit(-1)

    # Load dataset
    dataset = parse_directory(sys.argv[1])
    # Compute accuracy of the v2p pipeline
    results = compute_accuracy(dataset, v2p_pipeline)

    # Summary:
    print("-" * 75)
    print("| Summary: ")
    print("|")
    mean_pitch_error = sum([r["midi_number_distance"] for r in results.values()]) / len(results)
    print("| Mean Pitch Accuracy:\t\t", 100 * (1 - mean_pitch_error), "%")
    mean_segmentation_error = sum([r["note_segmentation_distance"] for r in results.values()]) / len(results)
    print("| Mean Segmentation Accuracy:\t", 100 * (1 - mean_segmentation_error), "%")
    print("-" * 75)


    # Display detailed results
    for fileid, result in results.items():
        print("+", fileid, ":")
        print("| Pitch Accuracy:\t\t", 100 * (1 - result["midi_number_distance"]), "%")
        print("| Segmentation Accuracy:\t", 100 * (1 - result["note_segmentation_distance"]), "%")
        print("| Ghosts:", result["ghost_notes"])
    print("-" * 75)
