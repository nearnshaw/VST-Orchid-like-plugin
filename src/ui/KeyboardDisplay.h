#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Draws a two-octave piano keyboard (C3–B4), highlighting the active root note.
// Also displays the current chord name.
class KeyboardDisplay : public juce::Component
{
public:
    KeyboardDisplay();

    void setRootNote   (int midiNote);           // -1 = none
    void setChordName  (const juce::String& name);
    void setChordNotes (uint16_t notesBitmask);  // bit i = pitch class i is sounding

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    int           rootNote       = -1;
    juce::String  chordName;
    uint16_t      chordNotesMask = 0;   // pitch classes currently sounding

    // Layout computed in resized()
    struct KeyRect { int midiNote; juce::Rectangle<float> bounds; bool isBlack; };
    std::vector<KeyRect> keyRects;

    void buildKeyLayout();

    static const juce::Colour kBgColour;
    static const juce::Colour kWhiteKey;
    static const juce::Colour kBlackKey;
    static const juce::Colour kRootColour;
    static const juce::Colour kChordNoteWhite;  // chord note, white key
    static const juce::Colour kChordNoteBlack;  // chord note, black key
    static const juce::Colour kTextColour;
};
