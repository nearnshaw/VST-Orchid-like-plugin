#include "ChordEngine.h"
#include <algorithm>
#include <string>

const int ChordEngine::kMajorIntervals[3] = { 0, 4, 7 };
const int ChordEngine::kMinorIntervals[3] = { 0, 3, 7 };
const int ChordEngine::kSus4Intervals[3]  = { 0, 5, 7 };
const int ChordEngine::kDimIntervals[3]   = { 0, 3, 6 };

std::vector<int> ChordEngine::buildIntervals(uint8_t chordType, uint8_t extensions) const
{
    std::vector<int> intervals;

    // Base triad
    const int* base = nullptr;
    bool isDim = false;

    if (chordType & ChordType::Major)      { base = kMajorIntervals; }
    else if (chordType & ChordType::Minor) { base = kMinorIntervals; }
    else if (chordType & ChordType::Sus4)  { base = kSus4Intervals; }
    else if (chordType & ChordType::Dim)   { base = kDimIntervals; isDim = true; }

    // Root is always present
    intervals.push_back(0);

    // If a chord type is set, add its triad intervals
    if (base != nullptr)
        for (int i = 1; i < 3; ++i)   // skip index 0 (root already added)
            intervals.push_back(base[i]);

    // Extensions
    if (extensions & Extension::Add6)
        intervals.push_back(9);

    if (extensions & Extension::AddM7)
    {
        // Dim + m7 = fully diminished 7 (interval +9, same as Add6 but distinct extension)
        // For dim, use diminished 7th (+9); for others, use minor 7th (+10)
        intervals.push_back(isDim ? 9 : 10);
    }

    if (extensions & Extension::AddMaj7)
        intervals.push_back(11);

    if (extensions & Extension::Add9)
        intervals.push_back(14);

    // Remove duplicates and sort
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());

    return intervals;
}

std::vector<int> ChordEngine::buildNotes(int rootMidi, uint8_t chordType, uint8_t extensions) const
{
    auto intervals = buildIntervals(chordType, extensions);
    std::vector<int> notes;
    notes.reserve(intervals.size());
    for (int iv : intervals)
        notes.push_back(rootMidi + iv);
    return notes;
}

std::string ChordEngine::getChordName(int rootMidi, uint8_t chordType, uint8_t extensions)
{
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    std::string name = noteNames[rootMidi % 12];

    if (chordType & ChordType::Major)
    {
        name += "maj";
        if (extensions & Extension::AddMaj7) name += "7";
        else if (extensions & Extension::AddM7) name += " dom7";
        if (extensions & Extension::Add9)  name += "9";
        if (extensions & Extension::Add6)  name += "6";
    }
    else if (chordType & ChordType::Minor)
    {
        name += "m";
        if (extensions & Extension::AddMaj7) name += "\u03947";   // mΔ7
        else if (extensions & Extension::AddM7) name += "7";
        if (extensions & Extension::Add9)  name += "9";
        if (extensions & Extension::Add6)  name += "6";
    }
    else if (chordType & ChordType::Sus4)
    {
        name += "sus4";
        if (extensions & Extension::AddM7)  name += "7";
        if (extensions & Extension::AddMaj7) name += "maj7";
        if (extensions & Extension::Add9)   name += "9";
    }
    else if (chordType & ChordType::Dim)
    {
        name += "dim";
        if ((extensions & Extension::AddM7) || (extensions & Extension::Add6))
            name += "7";
        if (extensions & Extension::Add9) name += "9";
    }
    else
    {
        // No chord type
        return name;
    }

    return name;
}
