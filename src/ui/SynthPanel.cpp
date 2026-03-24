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
        for (juce::Component* c : { (juce::Component*)&engineLabel,    (juce::Component*)&engineCombo,
                                    (juce::Component*)&bassEngineLabel, (juce::Component*)&bassEngineCombo,
                                    (juce::Component*)&attackLabel,    (juce::Component*)&attackSlider,
                                    (juce::Component*)&decayLabel,     (juce::Component*)&decaySlider,
                                    (juce::Component*)&sustainLabel,   (juce::Component*)&sustainSlider,
                                    (juce::Component*)&releaseLabel,   (juce::Component*)&releaseSlider,
                                    (juce::Component*)&cutoffLabel,    (juce::Component*)&cutoffSlider,
                                    (juce::Component*)&resonanceLabel, (juce::Component*)&resonanceSlider,
                                    (juce::Component*)&gainLabel,      (juce::Component*)&gainSlider,
                                    (juce::Component*)&perfModeLabel,  (juce::Component*)&perfModeCombo })
            c->setVisible(!collapsed);

        if (!collapsed)
            updatePerfVisibility();
        else
        {
            for (juce::Component* c : { (juce::Component*)&strumSpeedLabel, (juce::Component*)&strumSpeedCombo,
                                        (juce::Component*)&strumUpToggle,
                                        (juce::Component*)&arpPatternLabel, (juce::Component*)&arpPatternCombo,
                                        (juce::Component*)&arpBpmLabel,     (juce::Component*)&arpBpmSlider })
                c->setVisible(false);
        }

        if (onToggle) onToggle();
    };
    addAndMakeVisible(collapseBtn);

    // --- Engine combos ---
    auto addEngineItems = [](juce::ComboBox& cb) {
        cb.addItem("Piano",   1);
        cb.addItem("Pad",     2);
        cb.addItem("Synth",   3);
        cb.addItem("Strings", 4);
        cb.addItem("Pluck",   5);
        cb.addItem("Organ",   6);
    };

    addEngineItems(engineCombo);
    engineCombo.setSelectedId(1);
    engineAttachment = std::make_unique<ComboAtt>(apvts, ParamIDs::SynthEngineType, engineCombo);

    addEngineItems(bassEngineCombo);
    bassEngineCombo.setSelectedId(4); // default: Strings
    bassEngineAttachment = std::make_unique<ComboAtt>(apvts, ParamIDs::BassEngineType, bassEngineCombo);

    // Audio on/off toggle
    audioToggle.setColour(juce::ToggleButton::textColourId, kTextColour.withAlpha(0.8f));
    audioToggle.setColour(juce::ToggleButton::tickColourId, kAccentColour);
    audioAtt = std::make_unique<ButtonAtt>(apvts, ParamIDs::SynthEnabled, audioToggle);
    addAndMakeVisible(audioToggle);

    // --- Sliders ---
    setupSlider(attackSlider,    juce::Slider::RotaryVerticalDrag);
    setupSlider(decaySlider,     juce::Slider::RotaryVerticalDrag);
    setupSlider(sustainSlider,   juce::Slider::RotaryVerticalDrag);
    setupSlider(releaseSlider,   juce::Slider::RotaryVerticalDrag);
    setupSlider(cutoffSlider,    juce::Slider::LinearHorizontal);
    setupSlider(resonanceSlider, juce::Slider::RotaryVerticalDrag);
    setupSlider(gainSlider,      juce::Slider::RotaryVerticalDrag);
    arpBpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    arpBpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
    arpBpmSlider.setColour(juce::Slider::trackColourId, kAccentColour);
    arpBpmSlider.setColour(juce::Slider::textBoxTextColourId, kTextColour);
    arpBpmSlider.setColour(juce::Slider::textBoxBackgroundColourId, kBgColour);
    arpBpmSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(arpBpmSlider);

    // APVTS attachments
    attackAtt    = std::make_unique<SliderAtt>(apvts, ParamIDs::Attack,          attackSlider);
    decayAtt     = std::make_unique<SliderAtt>(apvts, ParamIDs::Decay,           decaySlider);
    sustainAtt   = std::make_unique<SliderAtt>(apvts, ParamIDs::Sustain,         sustainSlider);
    releaseAtt   = std::make_unique<SliderAtt>(apvts, ParamIDs::Release,         releaseSlider);
    cutoffAtt    = std::make_unique<SliderAtt>(apvts, ParamIDs::FilterCutoff,    cutoffSlider);
    resonanceAtt = std::make_unique<SliderAtt>(apvts, ParamIDs::FilterResonance, resonanceSlider);
    gainAtt      = std::make_unique<SliderAtt>(apvts, ParamIDs::OutputGain,      gainSlider);
    arpBpmAtt    = std::make_unique<SliderAtt>(apvts, ParamIDs::ArpBPM,          arpBpmSlider);

    // --- Labels ---
    for (auto* lbl : { &engineLabel, &bassEngineLabel, &attackLabel, &decayLabel, &sustainLabel,
                       &releaseLabel, &cutoffLabel, &resonanceLabel, &gainLabel,
                       &perfModeLabel, &strumSpeedLabel, &arpPatternLabel, &arpBpmLabel })
        setupLabel(*lbl);

    addAndMakeVisible(engineLabel);
    addAndMakeVisible(engineCombo);
    addAndMakeVisible(bassEngineLabel);
    addAndMakeVisible(bassEngineCombo);

    // --- Performance section ---
    perfModeCombo.addItem("Normal",      1);
    perfModeCombo.addItem("Strum",       2);
    perfModeCombo.addItem("Arpeggiator", 3);
    perfModeCombo.setSelectedId(1);
    perfModeAtt = std::make_unique<ComboAtt>(apvts, ParamIDs::PerfMode, perfModeCombo);
    perfModeCombo.onChange = [this]() { updatePerfVisibility(); if (onToggle) onToggle(); };
    addAndMakeVisible(perfModeLabel);
    addAndMakeVisible(perfModeCombo);

    // Strum controls
    strumSpeedCombo.addItem("Slow",   1);
    strumSpeedCombo.addItem("Medium", 2);
    strumSpeedCombo.addItem("Fast",   3);
    strumSpeedCombo.setSelectedId(2);
    strumSpeedAtt = std::make_unique<ComboAtt>(apvts, ParamIDs::StrumSpeed, strumSpeedCombo);
    addAndMakeVisible(strumSpeedLabel);
    addAndMakeVisible(strumSpeedCombo);

    strumUpToggle.setColour(juce::ToggleButton::textColourId, kTextColour.withAlpha(0.8f));
    strumUpToggle.setColour(juce::ToggleButton::tickColourId, kAccentColour);
    strumUpAtt = std::make_unique<ButtonAtt>(apvts, ParamIDs::StrumUp, strumUpToggle);
    addAndMakeVisible(strumUpToggle);

    // Arp controls
    arpPatternCombo.addItem("Up",       1);
    arpPatternCombo.addItem("Down",     2);
    arpPatternCombo.addItem("Up-Down",  3);
    arpPatternCombo.addItem("Down-Up",  4);
    arpPatternCombo.addItem("Random",   5);
    arpPatternCombo.setSelectedId(1);
    arpPatternAtt = std::make_unique<ComboAtt>(apvts, ParamIDs::ArpPattern, arpPatternCombo);
    addAndMakeVisible(arpPatternLabel);
    addAndMakeVisible(arpPatternCombo);
    addAndMakeVisible(arpBpmLabel);

    updatePerfVisibility();
}

void SynthPanel::updatePerfVisibility()
{
    const int mode = perfModeCombo.getSelectedId() - 1; // 0=Normal, 1=Strum, 2=Arp

    const bool showStrum = (mode == 1);
    const bool showArp   = (mode == 2);

    strumSpeedLabel.setVisible(showStrum);
    strumSpeedCombo.setVisible(showStrum);
    strumUpToggle.setVisible(showStrum);

    arpPatternLabel.setVisible(showArp);
    arpPatternCombo.setVisible(showArp);
    arpBpmLabel.setVisible(showArp);
    arpBpmSlider.setVisible(showArp);

    resized(); // re-layout to reclaim/allocate space
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
    const int btnW    = 20;
    const int toggleW = 62;
    collapseBtn.setBounds(w - btnW - 4, (kHeaderH - 18) / 2, btnW, 18);
    audioToggle.setBounds(w - btnW - 8 - toggleW, (kHeaderH - 18) / 2, toggleW, 18);

    if (collapsed) return;

    int y = kHeaderH + pad;

    // Chord engine
    engineLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    engineCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 28;

    // Bass engine
    bassEngineLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    bassEngineCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 28;

    // ADSR row
    const int knobSize = 40;
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

    // Resonance + Gain — fixed-size knobs so they don't dominate the panel
    const int halfW   = (w - pad * 3) / 2;
    const int knobH   = juce::jmin(halfW, 52);
    resonanceLabel.setBounds(pad, y, halfW, 12);
    gainLabel.setBounds(pad + halfW + pad, y, halfW, 12);
    y += 14;
    resonanceSlider.setBounds(pad, y, halfW, knobH);
    gainSlider.setBounds(pad + halfW + pad, y, halfW, knobH);
    y += knobH + pad;

    // Performance mode
    perfModeLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    perfModeCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 28;

    const int mode = perfModeCombo.getSelectedId() - 1;

    if (mode == 1) // Strum
    {
        // Speed combo + Up/Down toggle on same row
        const int speedW = (w - pad * 3) / 2;
        strumSpeedLabel.setBounds(pad, y, speedW, 12);
        strumSpeedCombo.setBounds(pad, y + 14, speedW, 22);
        strumUpToggle.setBounds(pad + speedW + pad, y + 14, speedW, 22);
    }
    else if (mode == 2) // Arp
    {
        arpPatternLabel.setBounds(pad, y, w - pad * 2, 12);
        y += 14;
        arpPatternCombo.setBounds(pad, y, w - pad * 2, 22);
        y += 28;
        arpBpmLabel.setBounds(pad, y, w - pad * 2, 12);
        y += 14;
        arpBpmSlider.setBounds(pad, y, w - pad * 2, 20);
    }
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
