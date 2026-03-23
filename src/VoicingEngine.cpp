#include "VoicingEngine.h"
#include <algorithm>
#include <cassert>

std::vector<int> VoicingEngine::applyVoicing(const std::vector<int>& intervals,
                                               int rootMidi,
                                               const VoicingParams& params) const
{
    if (intervals.empty())
        return {};

    std::vector<int> notes;

    switch (params.voicing)
    {
        case Voicing::Close:      notes = applyClose(intervals, rootMidi);      break;
        case Voicing::Inversion1: notes = applyInversion(intervals, rootMidi, 1); break;
        case Voicing::Inversion2: notes = applyInversion(intervals, rootMidi, 2); break;
        case Voicing::Inversion3: notes = applyInversion(intervals, rootMidi, 3); break;
        case Voicing::Open:       notes = applyOpen(intervals, rootMidi);       break;
        case Voicing::Drop2:      notes = applyDrop2(intervals, rootMidi);      break;
        case Voicing::Spread:     notes = applySpread(intervals, rootMidi);     break;
    }

    shiftOctave(notes, params.octaveOffset);
    notes = clampToRange(std::move(notes), params.midiRangeMin, params.midiRangeMax);
    std::sort(notes.begin(), notes.end());

    return notes;
}

int VoicingEngine::getBassNote(const std::vector<int>& voicedNotes) const
{
    if (voicedNotes.empty())
        return -1;
    return *std::min_element(voicedNotes.begin(), voicedNotes.end());
}

std::vector<int> VoicingEngine::applyClose(const std::vector<int>& intervals, int root) const
{
    // Simple: root + each interval, stacked from root
    std::vector<int> notes;
    for (int iv : intervals)
        notes.push_back(root + iv);
    return notes;
}

std::vector<int> VoicingEngine::applyInversion(const std::vector<int>& intervals,
                                                int root, int n) const
{
    if (intervals.empty())
        return {};

    // Start from close voicing
    std::vector<int> notes = applyClose(intervals, root);
    std::sort(notes.begin(), notes.end());

    // Number of available inversions = num notes - 1
    int maxInv = static_cast<int>(notes.size()) - 1;
    int actualN = std::min(n, maxInv);

    // For each inversion step, take the bottom note and raise it an octave
    for (int i = 0; i < actualN; ++i)
    {
        std::sort(notes.begin(), notes.end());
        notes.front() += 12;
    }

    std::sort(notes.begin(), notes.end());
    return notes;
}

std::vector<int> VoicingEngine::applyOpen(const std::vector<int>& intervals, int root) const
{
    // Start from close voicing, raise every other note (indices 1, 3, ...) by an octave
    std::vector<int> notes = applyClose(intervals, root);
    std::sort(notes.begin(), notes.end());

    for (size_t i = 1; i < notes.size(); i += 2)
        notes[i] += 12;

    std::sort(notes.begin(), notes.end());
    return notes;
}

std::vector<int> VoicingEngine::applyDrop2(const std::vector<int>& intervals, int root) const
{
    // Close voicing sorted descending; take the 2nd-highest and drop it an octave
    std::vector<int> notes = applyClose(intervals, root);
    std::sort(notes.begin(), notes.end());

    if (notes.size() < 2)
        return notes;

    // 2nd-highest = notes[size - 2]
    notes[notes.size() - 2] -= 12;
    std::sort(notes.begin(), notes.end());
    return notes;
}

std::vector<int> VoicingEngine::applySpread(const std::vector<int>& intervals, int root) const
{
    // Wide voicing: root (low) + 5th (low) + 3rd (high) + 7th (high)
    // If intervals don't have enough notes, fall back to Open
    if (intervals.size() < 3)
        return applyOpen(intervals, root);

    // Find key intervals (by closest to target semitone values)
    // Root is always intervals[0] = 0
    // Find the 5th (closest to 7), 3rd (closest to 3 or 4), and 7th/ext (>=10)
    std::vector<int> notes = applyClose(intervals, root);
    std::sort(notes.begin(), notes.end());

    if (notes.size() == 3)
    {
        // Place root at base, middle note up an octave, top note up an octave
        notes[1] += 12;
        notes[2] += 12;
    }
    else
    {
        // 4+ notes: bottom two stay low, top notes go up an octave
        for (size_t i = 2; i < notes.size(); ++i)
            notes[i] += 12;
    }

    std::sort(notes.begin(), notes.end());
    return notes;
}

std::vector<int> VoicingEngine::clampToRange(std::vector<int> notes,
                                              int minNote, int maxNote) const
{
    for (auto& n : notes)
    {
        while (n < minNote) n += 12;
        while (n > maxNote) n -= 12;
        n = std::clamp(n, minNote, maxNote);
    }

    // Remove duplicates after clamping
    std::sort(notes.begin(), notes.end());
    notes.erase(std::unique(notes.begin(), notes.end()), notes.end());
    return notes;
}

void VoicingEngine::shiftOctave(std::vector<int>& notes, int octaveOffset) const
{
    for (auto& n : notes)
        n += octaveOffset * 12;
}
