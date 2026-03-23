#include "VoicingPanel.h"
#include "../OrchidParameters.h"

const juce::Colour VoicingPanel::kBgColour  = juce::Colour(0xFF1A1A2E);
const juce::Colour VoicingPanel::kTextColour = juce::Colour(0xFFDDDDEE);

VoicingPanel::VoicingPanel(juce::AudioProcessorValueTreeState& apvts_)
    : apvts(apvts_)
{
    // Collapse button
    collapseBtn.setButtonText("v");
    collapseBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    collapseBtn.setColour(juce::TextButton::textColourOffId, kTextColour.withAlpha(0.7f));
    collapseBtn.onClick = [this]()
    {
        collapsed = !collapsed;
        collapseBtn.setButtonText(collapsed ? ">" : "v");
        for (juce::Component* c : { (juce::Component*)&voicingLabel, (juce::Component*)&voicingCombo,
                                    (juce::Component*)&octaveLabel,  (juce::Component*)&octaveCombo,
                                    (juce::Component*)&bassLabel,    (juce::Component*)&bassToggle })
            c->setVisible(!collapsed);
        if (onToggle) onToggle();
    };
    addAndMakeVisible(collapseBtn);

    // Voicing combo
    voicingCombo.addItem("Close",       1);
    voicingCombo.addItem("Inversion 1", 2);
    voicingCombo.addItem("Inversion 2", 3);
    voicingCombo.addItem("Inversion 3", 4);
    voicingCombo.addItem("Open",        5);
    voicingCombo.addItem("Drop 2",      6);
    voicingCombo.addItem("Spread",      7);
    voicingCombo.setSelectedId(1);
    voicingAttachment = std::make_unique<ComboAtt>(apvts, ParamIDs::VoicingType, voicingCombo);

    // Octave combo (-2 to +2)
    octaveCombo.addItem("-2", 1);
    octaveCombo.addItem("-1", 2);
    octaveCombo.addItem(" 0", 3);
    octaveCombo.addItem("+1", 4);
    octaveCombo.addItem("+2", 5);
    octaveCombo.setSelectedId(3);
    octaveAttachment = std::make_unique<ComboAtt>(apvts, ParamIDs::OctaveOffset, octaveCombo);

    bassAttachment = std::make_unique<ButtonAtt>(apvts, ParamIDs::BassEnabled, bassToggle);

    for (auto* lbl : { &voicingLabel, &octaveLabel, &bassLabel })
    {
        lbl->setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        lbl->setColour(juce::Label::textColourId, kTextColour.withAlpha(0.7f));
        addAndMakeVisible(lbl);
    }

    addAndMakeVisible(voicingCombo);
    addAndMakeVisible(octaveCombo);
    addAndMakeVisible(bassToggle);

    bassToggle.setColour(juce::ToggleButton::textColourId, kTextColour);
    bassToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFE06B3A));
}

void VoicingPanel::resized()
{
    const int w   = getWidth();
    const int pad = 8;

    // Header with title and collapse button
    const int btnW = 20;
    collapseBtn.setBounds(w - btnW - 4, (kHeaderH - 18) / 2, btnW, 18);

    if (collapsed) return;

    int y = kHeaderH + pad;

    voicingLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    voicingCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 30;

    octaveLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    octaveCombo.setBounds(pad, y, w - pad * 2, 22);
    y += 30;

    bassLabel.setBounds(pad, y, w - pad * 2, 14);
    y += 16;
    bassToggle.setBounds(pad, y, w - pad * 2, 22);
}

void VoicingPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Header bar
    g.setColour(kBgColour.brighter(0.12f));
    g.fillRect(0, 0, getWidth(), kHeaderH);

    g.setColour(kTextColour.withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText("VOICING", 8, 0, getWidth() - 36, kHeaderH, juce::Justification::centredLeft);

    g.setColour(kTextColour.withAlpha(0.15f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}
