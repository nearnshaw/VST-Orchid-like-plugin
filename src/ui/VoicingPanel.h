#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class VoicingPanel : public juce::Component
{
public:
    explicit VoicingPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label    voicingLabel  { {}, "VOICING" };
    juce::ComboBox voicingCombo;

    juce::Label    octaveLabel   { {}, "OCTAVE" };
    juce::ComboBox octaveCombo;

    juce::Label        bassLabel  { {}, "BASS CH" };
    juce::ToggleButton bassToggle { "Bass" };

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAtt>  voicingAttachment;
    std::unique_ptr<ComboAtt>  octaveAttachment;
    std::unique_ptr<ButtonAtt> bassAttachment;

    static const juce::Colour kBgColour;
    static const juce::Colour kTextColour;
};
