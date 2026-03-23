#pragma once
#include <vector>

class VoicingEngine
{
public:
    enum class Voicing : int
    {
        Close      = 0,   // Root position, stacked within one octave
        Inversion1 = 1,   // 3rd in bass
        Inversion2 = 2,   // 5th in bass
        Inversion3 = 3,   // 7th in bass (4-note chords only)
        Open       = 4,   // Alternating notes raised an octave
        Drop2      = 5,   // 2nd-highest note of close voicing dropped an octave
        Spread     = 6    // Root+5th low, 3rd+7th high
    };

    struct VoicingParams
    {
        Voicing voicing    = Voicing::Close;
        int octaveOffset   = 0;      // -2 to +2 shifts entire chord
        bool bassEnabled   = true;
        int midiRangeMin   = 36;     // C2
        int midiRangeMax   = 96;     // C7
    };

    // intervals: semitones above root (must include 0 for root)
    // rootMidi: MIDI note number for root
    std::vector<int> applyVoicing(const std::vector<int>& intervals,
                                  int rootMidi,
                                  const VoicingParams& params) const;

    // Returns the lowest note from a voiced chord (bass note)
    int getBassNote(const std::vector<int>& voicedNotes) const;

private:
    std::vector<int> applyClose     (const std::vector<int>& intervals, int root) const;
    std::vector<int> applyInversion (const std::vector<int>& intervals, int root, int n) const;
    std::vector<int> applyOpen      (const std::vector<int>& intervals, int root) const;
    std::vector<int> applyDrop2     (const std::vector<int>& intervals, int root) const;
    std::vector<int> applySpread    (const std::vector<int>& intervals, int root) const;
    std::vector<int> clampToRange   (std::vector<int> notes, int minNote, int maxNote) const;
    void shiftOctave                (std::vector<int>& notes, int octaveOffset) const;
};
