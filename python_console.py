#!/usr/bin/env python3 -i

# Load the v2p binding
from importlib.machinery import SourceFileLoader
SourceFileLoader("v2p", "./python/v2p/__init__.py").load_module()

# Import everything from the module
import v2p
from v2p import *

# Load python tests
SourceFileLoader("test", "./python/v2p-test/__init__.py").load_module()
from test import *

# Usefull modules
import math
import matplotlib.pyplot as plt
import numpy as np


# Load wav file and set some globals for easy debugging
def quick_load(filename):
    global audio, pitch, numbers, notes
    audio = wav_load(filename)
    pitch = boersma_path(audio)
    numbers = pitch_to_midi_numbers(pitch)
    notes = midi_numbers_to_notes(numbers)
