#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ChordButtonPanel.h"
#include "ui/VoicingPanel.h"
#include "ui/SynthPanel.h"
#include "ui/MidiConfigPanel.h"
#include "ui/KeyboardDisplay.h"
#include "ui/CircleOfFifthsDisplay.h"

class BegoniaEditor : public juce::AudioProcessorEditor,
                     public juce::KeyListener,
                     public juce::Timer
{
public:
    explicit BegoniaEditor(BegoniaProcessor&);
    ~BegoniaEditor() override;

    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override { grabKeyboardFocus(); }

    // juce::KeyListener
    bool keyPressed     (const juce::KeyPress& key, juce::Component* originator) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originator) override;

    // juce::Timer (30Hz UI refresh)
    void timerCallback() override;

private:
    BegoniaProcessor& processor;

    // Panels
    ChordButtonPanel       chordPanel;
    VoicingPanel           voicingPanel;
    SynthPanel             synthPanel;
    MidiConfigPanel        midiConfigPanel;
    KeyboardDisplay        keyboardDisplay;
    CircleOfFifthsDisplay  circleDisplay;

    // Header controls
    juce::Label        titleLabel  { {}, "BEGONIA" };
    juce::ToggleButton globalKeyBtn { "Global Key" };

    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAtt> globalKeyAtt;

    // Colour palette
    static const juce::Colour kBgColour;
    static const juce::Colour kHeaderBgColour;
    static const juce::Colour kTextColour;
    static const juce::Colour kAccentColour;

    // Layout constants
    static constexpr int kWidth        = 1100;
    static constexpr int kHeight       = 520;
    static constexpr int kHeaderH      = 36;
    static constexpr int kChordPanelW  = 170;
    static constexpr int kCirclePanelW = 370;
    static constexpr int kMidiBarH     = 52;
    static constexpr int kKeyboardH    = 70;

    void grabFocusSafely();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BegoniaEditor)
};
