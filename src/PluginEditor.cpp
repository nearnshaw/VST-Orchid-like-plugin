#include "PluginEditor.h"

const juce::Colour OrchidEditor::kBgColour      = juce::Colour(0xFF12121E);
const juce::Colour OrchidEditor::kHeaderBgColour = juce::Colour(0xFF0D0D1A);
const juce::Colour OrchidEditor::kTextColour     = juce::Colour(0xFFDDDDEE);
const juce::Colour OrchidEditor::kAccentColour   = juce::Colour(0xFFE06B3A);

OrchidEditor::OrchidEditor(OrchidProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      chordPanel(p.getKeyboardMapper()),
      voicingPanel(p.getAPVTS()),
      synthPanel(p.getAPVTS()),
      midiConfigPanel(p.getAPVTS())
{
    setSize(kWidth, kHeight);
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Header
    titleLabel.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    titleLabel.setColour(juce::Label::textColourId, kAccentColour);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    globalKeyBtn.setColour(juce::ToggleButton::textColourId, kTextColour.withAlpha(0.8f));
    globalKeyBtn.setColour(juce::ToggleButton::tickColourId, kAccentColour);
    globalKeyBtn.setButtonText("Global Keys");
    globalKeyAtt = std::make_unique<ButtonAtt>(p.getAPVTS(),
                                               ParamIDs::GlobalKeyMonitor,
                                               globalKeyBtn);
    addAndMakeVisible(globalKeyBtn);

    // Content panels
    addAndMakeVisible(chordPanel);
    addAndMakeVisible(voicingPanel);
    addAndMakeVisible(synthPanel);
    addAndMakeVisible(midiConfigPanel);
    addAndMakeVisible(keyboardDisplay);

    // Grab focus when clicked anywhere
    for (auto* comp : { (juce::Component*)&chordPanel,
                        (juce::Component*)&voicingPanel,
                        (juce::Component*)&synthPanel,
                        (juce::Component*)&midiConfigPanel,
                        (juce::Component*)&keyboardDisplay })
    {
        comp->addMouseListener(this, true);
    }

    // Start UI refresh timer at ~30 Hz
    startTimerHz(30);

    grabFocusSafely();
}

OrchidEditor::~OrchidEditor()
{
    stopTimer();
    removeKeyListener(this);
}

void OrchidEditor::grabFocusSafely()
{
    juce::MessageManager::callAsync([this]() {
        if (isVisible())
            grabKeyboardFocus();
    });
}

void OrchidEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    const int pad = 6;

    // Header strip
    titleLabel.setBounds(pad + 8, 0, 150, kHeaderH);
    globalKeyBtn.setBounds(w - 130, (kHeaderH - 20) / 2, 120, 20);

    // Main content area
    const int contentY = kHeaderH;
    const int contentH = h - kHeaderH - kMidiBarH - kKeyboardH;

    // Chord buttons panel (left)
    chordPanel.setBounds(pad, contentY + pad, kChordPanelW, contentH - pad * 2);

    // Remaining space for voicing + synth
    const int midX     = pad + kChordPanelW + pad;
    const int midW     = w - midX - pad;
    const int voicingW = midW * 2 / 5;
    const int synthW   = midW - voicingW - pad;

    voicingPanel.setBounds(midX, contentY + pad, voicingW, contentH - pad * 2);
    synthPanel.setBounds(midX + voicingW + pad, contentY + pad, synthW, contentH - pad * 2);

    // Keyboard display
    const int kbY = h - kMidiBarH - kKeyboardH;
    keyboardDisplay.setBounds(0, kbY, w, kKeyboardH);

    // MIDI config bar at bottom
    midiConfigPanel.setBounds(0, h - kMidiBarH, w, kMidiBarH);
}

void OrchidEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Header background
    g.setColour(kHeaderBgColour);
    g.fillRect(0, 0, getWidth(), kHeaderH);

    // Subtle bottom border on header
    g.setColour(kAccentColour.withAlpha(0.4f));
    g.fillRect(0, kHeaderH - 1, getWidth(), 1);
}

bool OrchidEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    return processor.getKeyboardMapper().handleKeyDown(key);
}

bool OrchidEditor::keyStateChanged(bool isKeyDown, juce::Component*)
{
    if (!isKeyDown)
    {
        // Release all keys that are no longer held by checking JUCE key state
        // We check each mapped key
        static const int keyCodes[] = { '1', '2', '3', '4', 'q', 'w', 'e', 'r',
                                         'Q', 'W', 'E', 'R' };
        for (int code : keyCodes)
        {
            if (!juce::KeyPress::isKeyCurrentlyDown(code))
                processor.getKeyboardMapper().handleKeyUp(code);
        }
    }
    return false;
}

void OrchidEditor::timerCallback()
{
    // Refresh chord button highlights
    chordPanel.refresh();

    // Update keyboard display
    keyboardDisplay.setRootNote(processor.getLastRootNote());
    keyboardDisplay.setChordName(processor.getLastChordName());

    // Re-grab focus if we lost it (e.g. user clicked back into plugin window)
    if (isVisible() && !hasKeyboardFocus(true))
    {
        // Only grab if mouse is over us (avoid stealing focus while user is elsewhere)
        if (isMouseOverOrDragging(true))
            grabKeyboardFocus();
    }
}
