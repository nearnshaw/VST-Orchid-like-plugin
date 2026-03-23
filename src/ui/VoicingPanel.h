#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class VoicingPanel : public juce::Component
{
public:
    explicit VoicingPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint   (juce::Graphics&) override;
    void resized () override;

    bool isCollapsed() const { return collapsed; }
    std::function<void()> onToggle;   // set by editor; called when collapsed state changes

    static constexpr int kHeaderH    = 28;
    static constexpr int kMinHeight  = kHeaderH;

private:
    juce::AudioProcessorValueTreeState& apvts;

    bool              collapsed   = false;
    juce::TextButton  collapseBtn { "v" };

    juce::Label    voicingLabel  { {}, "VOICING" };
    juce::ComboBox voicingCombo;

    juce::Label    octaveLabel   { {}, "OCTAVE" };
    juce::ComboBox octaveCombo;

    juce::Label        bassLabel  { {}, "BASS CH" };
    juce::ToggleButton bassToggle { "Bass" };

    juce::ToggleButton keyModeToggle { "Key Mode" };

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAtt>  voicingAttachment;
    std::unique_ptr<ComboAtt>  octaveAttachment;
    std::unique_ptr<ButtonAtt> bassAttachment;
    std::unique_ptr<ButtonAtt> keyModeAttachment;

    static const juce::Colour kBgColour;
    static const juce::Colour kTextColour;
};
