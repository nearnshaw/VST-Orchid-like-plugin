#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MidiConfigPanel : public juce::Component
{
public:
    explicit MidiConfigPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label    chordChLabel  { {}, "CHORD CH" };
    juce::ComboBox chordChCombo;

    juce::Label    bassChLabel   { {}, "BASS CH" };
    juce::ComboBox bassChCombo;

    juce::Label        midiOutLabel  { {}, "MIDI OUT" };
    juce::ToggleButton midiOutToggle { "MIDI Out" };

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAtt>  chordChAtt;
    std::unique_ptr<ComboAtt>  bassChAtt;
    std::unique_ptr<ButtonAtt> midiOutAtt;

    static const juce::Colour kBgColour;
    static const juce::Colour kTextColour;
};
