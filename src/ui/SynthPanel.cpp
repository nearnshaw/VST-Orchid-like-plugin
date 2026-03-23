#include "SynthPanel.h"
#include "../OrchidParameters.h"

const juce::Colour SynthPanel::kBgColour    = juce::Colour(0xFF1A1A2E);
const juce::Colour SynthPanel::kTextColour  = juce::Colour(0xFFDDDDEE);
const juce::Colour SynthPanel::kAccentColour = juce::Colour(0xFFE06B3A);

SynthPanel::SynthPanel(juce::AudioProcessorValueTreeState& apvts_) : apvts(apvts_)
{
    // Collapse button
    collapseBtn.setButtonText("v");
    collapseBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    collapseBtn.setColour(juce::TextButton::textColourOffId, kTextColour.withAlpha(0.7f));
    collapseBtn.onClick = [this]()
    {
        collapsed = !collapsed;
        collapseBtn.setButtonText(collapsed ? ">" : "v");
        for (juce::Component* c : { (juce::Component*)&engineLabel,   (juce::Component*)&engineCombo,
                                    (juce::Component*)&attackLabel,   (juce::Component*)&attackSlider,
                                    (juce::Component*)&decayLabel,    (juce::Component*)&decaySlider,
                                    (juce::Component*)&sustainLabel,  (juce::Component*)&sustainSlider,
                                    (juce::Component*)&releaseLabel,  (juce::Component*)&releaseSlider,
                                    (juce::Component*)&cutoffLabel,   (juce::Component*)&cutoffSlider,
                                    (juce::Component*)&resonanceLabel,(juce::Component*)&resonanceSlider,
                                    (juce::Component*)&gainLabel,     (juce::Component*)&gainSlider })
            c->setVisible(!collapsed);
        if (onToggle) onToggle();
    };
    addAndMakeVisible(collapseBtn);

    // Engine combo
    engineCombo.addItem("Piano", 1);
    engineCombo.addItem("Pad",   2);
    engineCombo.addItem("Synth", 3);
    engineCombo.setSelectedId(1);
    engineAttachment = std::make_unique<ComboAtt>(apvts, ParamIDs::SynthEngineType, engineCombo);

    // Sliders
    setupSlider(attackSlider,    juce::Slider::RotaryVerticalDrag);
    setupSlider(decaySlider,     juce::Slider::RotaryVerticalDrag);
    setupSlider(sustainSlider,   juce::Slider::RotaryVerticalDrag);
    setupSlider(releaseSlider,   juce::Slider::RotaryVerticalDrag);
    setupSlider(cutoffSlider,    juce::Slider::LinearHorizontal);
    setupSlider(resonanceSlider, juce::Slider::RotaryVerticalDrag);
    setupSlider(gainSlider,      juce::Slider::RotaryVerticalDrag);

    // APVTS attachments
    attackAtt    = std::make_unique<SliderAtt>(apvts, ParamIDs::Attack,          attackSlider);
    decayAtt     = std::make_unique<SliderAtt>(apvts, ParamIDs::Decay,           decaySlider);
    sustainAtt   = std::make_unique<SliderAtt>(apvts, ParamIDs::Sustain,         sustainSlider);
    releaseAtt   = std::make_unique<SliderAtt>(apvts, ParamIDs::Release,         releaseSlider);
    cutoffAtt    = std::make_unique<SliderAtt>(apvts, ParamIDs::FilterCutoff,    cutoffSlider);
    resonanceAtt = std::make_unique<SliderAtt>(apvts, ParamIDs::FilterResonance, resonanceSlider);
    gainAtt      = std::make_unique<SliderAtt>(apvts, ParamIDs::OutputGain,      gainSlider);

    // Labels
    for (auto* lbl : { &engineLabel, &attackLabel, &decayLabel, &sustainLabel,
                       &releaseLabel, &cutoffLabel, &resonanceLabel, &gainLabel })
        setupLabel(*lbl);

    addAndMakeVisible(engineLabel);
    addAndMakeVisible(engineCombo);
}

void SynthPanel::setupSlider(juce::Slider& s, juce::Slider::SliderStyle style)
{
    s.setSliderStyle(style);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::rotarySliderFillColourId, kAccentColour);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kTextColour.withAlpha(0.2f));
    s.setColour(juce::Slider::trackColourId, kAccentColour);
    addAndMakeVisible(s);
}

void SynthPanel::setupLabel(juce::Label& l)
{
    l.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    l.setColour(juce::Label::textColourId, kTextColour.withAlpha(0.7f));
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void SynthPanel::resized()
{
    const int w   = getWidth();
    const int pad = 6;

    // Header
    const int btnW = 20;
    collapseBtn.setBounds(w - btnW - 4, (kHeaderH - 18) / 2, btnW, 18);

    if (collapsed) return;

    int y = kHeaderH + pad;

    engineLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    engineCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 30;

    // ADSR row
    const int knobSize = 44;
    const int knobGap  = (w - pad * 2 - knobSize * 4) / 3;
    int x = pad;

    for (auto [knob, lbl] : std::initializer_list<std::pair<juce::Slider*, juce::Label*>>{
            {&attackSlider, &attackLabel}, {&decaySlider, &decayLabel},
            {&sustainSlider, &sustainLabel}, {&releaseSlider, &releaseLabel}})
    {
        lbl->setBounds(x, y, knobSize, 12);
        knob->setBounds(x, y + 13, knobSize, knobSize);
        x += knobSize + knobGap;
    }
    y += knobSize + 13 + pad;

    // Filter cutoff
    cutoffLabel.setBounds(pad, y, w - pad * 2, 12);
    y += 14;
    cutoffSlider.setBounds(pad, y, w - pad * 2, 18);
    y += 22;

    // Resonance + Gain
    const int halfW = (w - pad * 3) / 2;
    resonanceLabel.setBounds(pad, y, halfW, 12);
    gainLabel.setBounds(pad + halfW + pad, y, halfW, 12);
    y += 14;
    resonanceSlider.setBounds(pad, y, halfW, halfW);
    gainSlider.setBounds(pad + halfW + pad, y, halfW, halfW);
}

void SynthPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Header bar
    g.setColour(kBgColour.brighter(0.12f));
    g.fillRect(0, 0, getWidth(), kHeaderH);

    g.setColour(kTextColour.withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText("SYNTH", 8, 0, getWidth() - 36, kHeaderH, juce::Justification::centredLeft);

    g.setColour(kTextColour.withAlpha(0.15f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}
