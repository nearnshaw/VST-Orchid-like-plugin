#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SynthPanel : public juce::Component
{
public:
    explicit SynthPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint   (juce::Graphics&) override;
    void resized () override;

    bool isCollapsed() const { return collapsed; }
    std::function<void()> onToggle;

    static constexpr int kHeaderH   = 28;
    static constexpr int kMinHeight = kHeaderH;

private:
    juce::AudioProcessorValueTreeState& apvts;

    bool              collapsed   = false;
    juce::TextButton  collapseBtn { "v" };

    // Engine selector
    juce::Label    engineLabel { {}, "CHORD ENGINE" };
    juce::ComboBox engineCombo;

    // Bass engine selector
    juce::Label    bassEngineLabel { {}, "BASS ENGINE" };
    juce::ComboBox bassEngineCombo;

    // ADSR sliders
    juce::Label  attackLabel  { {}, "A" };
    juce::Slider attackSlider;
    juce::Label  decayLabel   { {}, "D" };
    juce::Slider decaySlider;
    juce::Label  sustainLabel { {}, "S" };
    juce::Slider sustainSlider;
    juce::Label  releaseLabel { {}, "R" };
    juce::Slider releaseSlider;

    // Filter
    juce::Label  cutoffLabel     { {}, "CUTOFF" };
    juce::Slider cutoffSlider;
    juce::Label  resonanceLabel  { {}, "RES" };
    juce::Slider resonanceSlider;

    // Gain
    juce::Label  gainLabel  { {}, "GAIN" };
    juce::Slider gainSlider;

    // Audio on/off toggle in header
    juce::ToggleButton audioToggle { "Audio" };

    // Performance mode
    juce::Label    perfModeLabel { {}, "PERFORMANCE" };
    juce::ComboBox perfModeCombo;

    // Strum controls
    juce::Label        strumSpeedLabel { {}, "SPEED" };
    juce::ComboBox     strumSpeedCombo;
    juce::ToggleButton strumUpToggle   { "Up" };

    // Arp controls
    juce::Label    arpPatternLabel { {}, "PATTERN" };
    juce::ComboBox arpPatternCombo;
    juce::Label    arpBpmLabel     { {}, "BPM" };
    juce::Slider   arpBpmSlider;

    using SliderAtt  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt   = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt  = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAtt>  engineAttachment;
    std::unique_ptr<ComboAtt>  bassEngineAttachment;
    std::unique_ptr<ButtonAtt> audioAtt;
    std::unique_ptr<SliderAtt> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<SliderAtt> cutoffAtt, resonanceAtt, gainAtt;
    std::unique_ptr<ComboAtt>  perfModeAtt;
    std::unique_ptr<ComboAtt>  strumSpeedAtt;
    std::unique_ptr<ButtonAtt> strumUpAtt;
    std::unique_ptr<ComboAtt>  arpPatternAtt;
    std::unique_ptr<SliderAtt> arpBpmAtt;

    void setupSlider (juce::Slider& s, juce::Slider::SliderStyle style);
    void setupLabel  (juce::Label& l);
    void updatePerfVisibility();

    static const juce::Colour kBgColour;
    static const juce::Colour kTextColour;
    static const juce::Colour kAccentColour;
};
