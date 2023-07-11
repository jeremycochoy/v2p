#!/usr/bin/env python3

from mido import Message, MetaMessage, MidiFile, MidiTrack

# Retrieve filename
import sys
filename = sys.argv[1]

mid = MidiFile(filename)
time = 0
notes = []
noteHolded = False
for msg in mid:
    print(msg)
    if type(msg) != Message:
        continue
    if msg.type == "note_on":
        if noteHolded == True:
            notes[-1] += [msg.time]
            noteHolded = False
        time += msg.time
        notes += [[msg.note, time]]
        noteHolded = True
    if msg.type == "note_off":
        time += msg.time
        if noteHolded == True:
            notes[-1] += [msg.time]
            noteHolded = False
#print(notes)
print([(w[0], w[1], w[2]) for w in notes])
