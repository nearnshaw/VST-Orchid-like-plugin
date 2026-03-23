#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../ChordEngine.h"
#include <array>
#include <cstdint>

class CircleOfFifthsDisplay : public juce::Component
{
public:
    explicit CircleOfFifthsDisplay(juce::AudioProcessorValueTreeState& apvts_);

    // Called from timerCallback (message thread)
    void setCurrentChord(int rootPitchClass, uint8_t chordType, uint16_t notesBitmask);
    void clearChord();

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    // ---------------------------------------------------------------
    // Circle-of-fifths data (12 positions, clockwise from C at top)
    // ---------------------------------------------------------------
    struct CoFPos
    {
        int majorRoot, minorRoot, dimRoot;
        juce::String majorName, minorName, dimName;
    };
    static const std::array<CoFPos, 12> kPos;

    // Maps pitch class (0-11) → circle position index
    // e.g. C(0)→0, G(7)→1, D(2)→2 …
    static const std::array<int, 12> kPCtoPos;

    // ---------------------------------------------------------------
    // State
    // ---------------------------------------------------------------
    int      activeRoot = -1;    // pitch class of playing chord root, -1 = none
    uint8_t  activeType = 0;     // ChordEngine::ChordType bitmask
    uint16_t activeMask = 0;     // bit i = pitch class i is in the chord

    // ---------------------------------------------------------------
    // Key selector
    // ---------------------------------------------------------------
    juce::AudioProcessorValueTreeState& apvts;
    juce::Label    keyLabel  { {}, "KEY" };
    juce::ComboBox keyCombo;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAtt> keyAtt;

    // Cached key pitch class (0=C … 11=B) — updated from onChange so paint()
    // never has to call into combo/APVTS internals.  Initialized after the
    // attachment fires its sendInitialUpdate(), so it's always valid.
    int m_key = 0;

    // ---------------------------------------------------------------
    // Geometry (set in resized)
    // ---------------------------------------------------------------
    juce::Point<float> centre;
    float outerR = 0.0f;

    // Ring outer-radius fractions (of outerR)
    static constexpr float kMajOuter = 1.00f;
    static constexpr float kMajInner = 0.72f;
    static constexpr float kMinOuter = 0.72f;
    static constexpr float kMinInner = 0.47f;
    static constexpr float kDimOuter = 0.47f;
    static constexpr float kDimInner = 0.30f;

    // ---------------------------------------------------------------
    // Drawing helpers
    // ---------------------------------------------------------------
    juce::Path          makeSegment  (float a1, float a2, float r1, float r2) const;
    juce::Point<float>  arcMidpoint  (float a1, float a2, float r) const;

    // ring: 0=major outer, 1=minor middle, 2=dim inner; key: 0=C … 11=B
    juce::Colour        segmentColour(int pos, int ring, int key) const;

    void drawSegmentLabel(juce::Graphics& g, const juce::String& text,
                          juce::Point<float> pt, float size, bool bold,
                          juce::Colour colour, float textBoxWidth = 52.0f) const;

    // ---------------------------------------------------------------
    // Music logic
    // ---------------------------------------------------------------
    int  selectedKey()               const;
    bool isDiatonic  (int pc, int ring) const;
    bool isActive    (int pos, int ring) const;
    bool isChordNote (int pc)           const;

    // ---------------------------------------------------------------
    // Colours
    // ---------------------------------------------------------------
    // [ring]: 0=major, 1=minor, 2=dim
    static const juce::Colour kBg;
    static const juce::Colour kBase     [3];
    static const juce::Colour kDiatonic [3];
    static const juce::Colour kTonic;           // I chord when no active chord
    static const juce::Colour kActive   [3];
    static const juce::Colour kNoteHint;
    static const juce::Colour kSep;
    static const juce::Colour kText     [3];
    static const juce::Colour kTextActive;
    static const juce::Colour kTextDiatonic;
    static const juce::Colour kTextTonic;
};
