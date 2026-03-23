#pragma once
#include <vector>
#include <cstdint>
#include <string>

class ChordEngine
{
public:
    enum ChordType : uint8_t
    {
        None  = 0,
        Major = 1 << 0,
        Minor = 1 << 1,
        Sus4  = 1 << 2,
        Dim   = 1 << 3
    };

    enum Extension : uint8_t
    {
        ExtNone  = 0,
        Add6     = 1 << 0,   // major 6th (+9 semitones)
        AddM7    = 1 << 1,   // dominant 7th (+10)
        AddMaj7  = 1 << 2,   // major 7th (+11)
        Add9     = 1 << 3    // major 9th (+14, kept above octave)
    };

    // Returns intervals as semitones above root (root = 0 always included).
    // chordType: single ChordType bit (last-pressed-wins resolved before calling)
    // extensions: bitmask of Extension flags
    std::vector<int> buildIntervals(uint8_t chordType, uint8_t extensions) const;

    // Convenience: resolve to absolute MIDI notes
    std::vector<int> buildNotes(int rootMidi, uint8_t chordType, uint8_t extensions) const;

    // Human-readable chord name (e.g. "Cmaj7", "Dm9")
    static std::string getChordName(int rootMidi, uint8_t chordType, uint8_t extensions);

private:
    static const int kMajorIntervals[3];
    static const int kMinorIntervals[3];
    static const int kSus4Intervals[3];
    static const int kDimIntervals[3];
};
