#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ChordButtonPanel.h"
#include "ui/VoicingPanel.h"
#include "ui/SynthPanel.h"
#include "ui/MidiConfigPanel.h"
#include "ui/KeyboardDisplay.h"

class OrchidEditor : public juce::AudioProcessorEditor,
                     public juce::KeyListener,
                     public juce::Timer
{
public:
    explicit OrchidEditor(OrchidProcessor&);
    ~OrchidEditor() override;

    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override { grabKeyboardFocus(); }

    // juce::KeyListener
    bool keyPressed     (const juce::KeyPress& key, juce::Component* originator) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originator) override;

    // juce::Timer (30Hz UI refresh)
    void timerCallback() override;

private:
    OrchidProcessor& processor;

    // Panels
    ChordButtonPanel  chordPanel;
    VoicingPanel      voicingPanel;
    SynthPanel        synthPanel;
    MidiConfigPanel   midiConfigPanel;
    KeyboardDisplay   keyboardDisplay;

    // Header controls
    juce::Label        titleLabel  { {}, "ORCHID" };
    juce::ToggleButton globalKeyBtn { "Global Key" };

    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAtt> globalKeyAtt;

    // Colour palette
    static const juce::Colour kBgColour;
    static const juce::Colour kHeaderBgColour;
    static const juce::Colour kTextColour;
    static const juce::Colour kAccentColour;

    // Layout constants
    static constexpr int kWidth       = 820;
    static constexpr int kHeight      = 500;
    static constexpr int kHeaderH     = 36;
    static constexpr int kChordPanelW = 130;
    static constexpr int kMidiBarH    = 52;
    static constexpr int kKeyboardH   = 70;

    void grabFocusSafely();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchidEditor)
};
