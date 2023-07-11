#V2p API

## Build

### Linux and MacOS
Build with:
```
mkdir build
cd build

cmake ..
make
```

### Windows
Build with:
```
mkdir build
cd build

cmake -G "Visual Studio 15 2017 Win64" ..
```
And use visual studio.

## The v2p pipeline

The audio signal go through divers transformation steps before
ending to a midi file. The key steps are known as
`audio` for raw audio wav file, `pitch` for the frequency across time,
`numbers` for the keybord note number across time, and `notes` for
a list of notes with a starting time, duration, velocity, and keyboard note number.
You can plug yourself and interfer at different stages of the process thanks
to the python binding. This allow testing new pitch correction methods, visualizing
the intermediate outputs and debugging fresh implementations.

Notice that the plot of the midi numbers curve will look very similar to the plot
of the pitch curve. The only change is in the scale (application of a log2 and
shifting to have A4 equal to the midi note number 69).

## Python binding

It is expected that the compiled shared library
(`v2p-api.dll` or `v2p-api.so`) is duplicated in the
`python/v2p/` directory.

### Install

The following python modules are required:
```
brew install make
brew install python3
python3 -m pip install numpy
python3 -m pip install mido
python3 -m pip install matplotlib
python3 -m pip install scipy

```

### Quick console
For a quick console with the v2p module imported, run `python3 -i python_console.py`

### Load python module
To load python module in your interpreter with the project as
working directory, you can simply run:
```
from importlib.machinery import SourceFileLoader
SourceFileLoader("v2p", "./python/v2p/__init__.py").load_module()

import v2p
```


## Usage

Launch the v2p python console.

## Load a wav file
```
audio = wav_load("mon_fichier.wav")
```

Since `audio` is a numpy array, you can plot it using pyplot:
```
plt.plot(audio)
plt.show()
```

## Compute the pitch path
The sampling of pitch in function of time can be computed using the boersma algorithm from the audio by
```
pitch = boersma_path(audio)
```

You can also plot the signal:
```
plt.plot(pitch)
plt.show()
```

The sampling rate can be changed. By defaut one sample is mesured each 10ms (0.01 seconds).
You can set it to 5ms with:
```
pitch = boersma_path(audio, timesteps=0.005)
```

## Display the pitch and midi segmentation
The segmentation into midi note of the signal can be visualized using the command
```
show_midi_segmentation(pitch)
```

## Midi Numbers
Midi numbers are just a logarithmic rescale of the pitch
curve. The Value 69 correspond to the note A4, with freauency 440Hz.
You can generate the midi note numbers from the pitch with
```
numbers = pitch_to_midi_numbers(pitch)
```

## Midi Notes
Midi notes are a representation of a note in the keyboard, with a
starting time (`position`), a `duration`, and a `velocity`.
Generate midi notes from numbers with:
```python
notes = midi_numbers_to_notes(numbers)

first_note = notes[0]
print("The first note have midi number equal to", note.note_number)
print("It's frequency is", midi_number_to_frequency(first_note.note_number))
print("The velocity is", first_note.velocity, "and the duration is", first_note.duration * 10, "ms")
```

## Correct the signal
Since not everybody sing like mikael jackson, you can try to align the pitch to the keyboard

Correction can happen at two stages: Midi Numbers and Midi Notes.

On midi numbers, the following operation are available:
```python
numbers_corrected = correct_numbers_grid(numbers)
numbers_corrected = correct_numbers_grid_using_mean_notes(numbers)
numbers_corrected = correct_numbers_grid_using_mean_notes_weighted(numbers)
## OUTDATED! They won't work anymore:
numbers_corrected = correct_numbers_grid_notes(numbers)
numbers_corrected = correct_numbers_grid_pondered_notes(numbers)
```

On midi notes, the following operations are available:
```python
## OUTDATED! They won't work anymore:
notes_corrected = correct_notes_grid(notes)
notes_corrected = correct_notes_intervals(notes)
notes_corrected = correct_notes_intervals_balanced(notes)
notes_corrected = correct_notes_intervals_harsh(notes)
```

## Convert to midi format

You can produce a midi file from notes. For faster use, you cand directly give
the pitch or the midi numbers. But the data won't be corrected in any way.
```python
pitch_to_midi(pitch, "filename.mid")
numbers_to_midi(numbers, "filename.mid")
notes_to_midi(notes, "filename.mid")
```

A real processing of the input would imply signal filtering and
pitch correction befor saving. For example, one could use the following process:
```python
audio = wav_load("source_file.wav")
pitch = boersma_pitch(audio)
numbers = pitch_to_midi_numbers(pitch)
numbers_filtered = median_filter(numbers, window_size=3)
numbers_corrected = correct_numbers_grid_using_mean_notes(numbers_filtered)
notes = midi_numbers_to_notes(numbers_corrected)
#notes_corrected = correct_notes_intervals(correct_notes_intervals(notes))
notes_to_midi(notes, "filename.mid")
```

## Easy debugging
A quick loading function was added. This function set the global variables
`audio`, `pitch`, `numbers` and `notes` from a clean and unfiltered
loading of a wav file.

```python
quick_load("my file.wav")
print(audio, pitch, numbers, notes)
```
