#include "MidiConfigPanel.h"
#include "../OrchidParameters.h"

const juce::Colour MidiConfigPanel::kBgColour   = juce::Colour(0xFF15152A);
const juce::Colour MidiConfigPanel::kTextColour = juce::Colour(0xFFDDDDEE);

MidiConfigPanel::MidiConfigPanel(juce::AudioProcessorValueTreeState& apvts_)
    : apvts(apvts_)
{
    // Populate MIDI channel combos 1–16
    for (int ch = 1; ch <= 16; ++ch)
    {
        chordChCombo.addItem("Ch " + juce::String(ch), ch);
        bassChCombo.addItem ("Ch " + juce::String(ch), ch);
    }
    chordChCombo.setSelectedId(1);
    bassChCombo.setSelectedId (2);

    chordChAtt = std::make_unique<ComboAtt>(apvts, ParamIDs::ChordChannel, chordChCombo);
    bassChAtt  = std::make_unique<ComboAtt>(apvts, ParamIDs::BassChannel,  bassChCombo);
    midiOutAtt = std::make_unique<ButtonAtt>(apvts, ParamIDs::MidiOutEnabled, midiOutToggle);

    for (auto* lbl : { &chordChLabel, &bassChLabel, &midiOutLabel })
    {
        lbl->setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        lbl->setColour(juce::Label::textColourId, kTextColour.withAlpha(0.7f));
        addAndMakeVisible(lbl);
    }

    addAndMakeVisible(chordChCombo);
    addAndMakeVisible(bassChCombo);
    addAndMakeVisible(midiOutToggle);

    midiOutToggle.setColour(juce::ToggleButton::textColourId, kTextColour);
    midiOutToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFE06B3A));
}

void MidiConfigPanel::resized()
{
    const int h   = getHeight();
    const int w   = getWidth();
    const int pad = 6;

    // Three equal sections
    const int secW = (w - pad * 4) / 3;
    int x = pad;

    chordChLabel.setBounds(x, pad, secW, 14);
    chordChCombo.setBounds(x, pad + 16, secW, 22);
    x += secW + pad;

    bassChLabel.setBounds(x, pad, secW, 14);
    bassChCombo.setBounds(x, pad + 16, secW, 22);
    x += secW + pad;

    midiOutLabel.setBounds(x, pad, secW, 14);
    midiOutToggle.setBounds(x, pad + 16, secW, 22);
}

void MidiConfigPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    g.setColour(kTextColour.withAlpha(0.1f));
    g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 1.0f);
}
