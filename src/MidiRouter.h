#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class MidiRouter
{
public:
    struct Config
    {
        int  chordChannel     = 1;    // 1-indexed MIDI channel for chord notes
        int  bassChannel      = 2;    // 1-indexed MIDI channel for bass note
        bool bassEnabled      = true;
        bool midiOutEnabled   = true;
        float velocity        = 0.8f; // 0.0–1.0
    };

    // Sends note-off for prevNotes and note-on for newNotes on correct channels.
    // bassNote is the lowest note of the chord, sent separately on bassChannel.
    void routeChord(juce::MidiBuffer& outBuffer,
                    const std::vector<int>& prevNotes,
                    const std::vector<int>& newNotes,
                    int bassNote,
                    const Config& config,
                    int samplePosition = 0) const;

    // Sends note-off for all notes in activeNotes on chord + bass channels
    void allNotesOff(juce::MidiBuffer& outBuffer,
                     const std::vector<int>& activeNotes,
                     int activeBassNote,
                     const Config& config,
                     int samplePosition = 0) const;
};
