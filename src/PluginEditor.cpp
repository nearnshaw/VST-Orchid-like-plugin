#include "PluginEditor.h"
#include "ui/VoicingPanel.h"
#include "ui/SynthPanel.h"

const juce::Colour BegoniaEditor::kBgColour       = juce::Colour(0xFF12121E);
const juce::Colour BegoniaEditor::kHeaderBgColour  = juce::Colour(0xFF0D0D1A);
const juce::Colour BegoniaEditor::kTextColour      = juce::Colour(0xFFDDDDEE);
const juce::Colour BegoniaEditor::kAccentColour    = juce::Colour(0xFFE06B3A);

BegoniaEditor::BegoniaEditor(BegoniaProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      chordPanel(p.getKeyboardMapper()),
      voicingPanel(p.getAPVTS()),
      synthPanel(p.getAPVTS()),
      midiConfigPanel(p.getAPVTS()),
      circleDisplay(p.getAPVTS())
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
    addAndMakeVisible(circleDisplay);

    // Re-layout when a panel collapses/expands
    voicingPanel.onToggle = [this]() { resized(); };
    synthPanel.onToggle   = [this]() { resized(); };

    // Grab focus when clicked anywhere
    for (auto* comp : { (juce::Component*)&chordPanel,
                        (juce::Component*)&voicingPanel,
                        (juce::Component*)&synthPanel,
                        (juce::Component*)&midiConfigPanel,
                        (juce::Component*)&keyboardDisplay,
                        (juce::Component*)&circleDisplay })
    {
        comp->addMouseListener(this, true);
    }

    startTimerHz(30);
    grabFocusSafely();
}

BegoniaEditor::~BegoniaEditor()
{
    stopTimer();
    removeKeyListener(this);
}

void BegoniaEditor::grabFocusSafely()
{
    juce::MessageManager::callAsync([this]() {
        if (isVisible())
            grabKeyboardFocus();
    });
}

void BegoniaEditor::resized()
{
    const int w   = getWidth();
    const int h   = getHeight();
    const int pad = 6;

    // Header strip
    titleLabel.setBounds(pad + 8, 0, 150, kHeaderH);
    globalKeyBtn.setBounds(w - 130, (kHeaderH - 20) / 2, 120, 20);

    // Content area (between header and keyboard/MIDI bar)
    const int contentY = kHeaderH;
    const int contentH = h - kHeaderH - kMidiBarH - kKeyboardH;

    // Left: chord buttons
    chordPanel.setBounds(pad, contentY + pad, kChordPanelW, contentH - pad * 2);

    // Right: circle of fifths
    const int circleX = w - kCirclePanelW - pad;
    circleDisplay.setBounds(circleX, contentY + pad, kCirclePanelW, contentH - pad * 2);

    // Middle: voicing + synth panels
    const int midX  = pad + kChordPanelW + pad;
    const int midW  = circleX - midX - pad;
    const int vW    = midW * 2 / 5;
    const int sW    = midW - vW - pad;

    const int vH = voicingPanel.isCollapsed() ? VoicingPanel::kMinHeight : contentH - pad * 2;
    const int sH = synthPanel.isCollapsed()   ? SynthPanel::kMinHeight   : contentH - pad * 2;
    voicingPanel.setBounds(midX,            contentY + pad, vW, vH);
    synthPanel.setBounds  (midX + vW + pad, contentY + pad, sW, sH);

    // Keyboard display (spans full width except circle panel)
    const int kbY = h - kMidiBarH - kKeyboardH;
    keyboardDisplay.setBounds(0, kbY, w - kCirclePanelW - pad, kKeyboardH);

    // MIDI config bar at bottom
    midiConfigPanel.setBounds(0, h - kMidiBarH, w, kMidiBarH);
}

void BegoniaEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Header background
    g.setColour(kHeaderBgColour);
    g.fillRect(0, 0, getWidth(), kHeaderH);

    // Accent line below header
    g.setColour(kAccentColour.withAlpha(0.4f));
    g.fillRect(0, kHeaderH - 1, getWidth(), 1);

    // Subtle divider between main panels and circle
    g.setColour(kBgColour.brighter(0.08f));
    g.fillRect(getWidth() - kCirclePanelW - 12, kHeaderH,
               1, getHeight() - kHeaderH - kMidiBarH);
}

bool BegoniaEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    return processor.getKeyboardMapper().handleKeyDown(key);
}

bool BegoniaEditor::keyStateChanged(bool isKeyDown, juce::Component*)
{
    if (!isKeyDown)
    {
        static const int keyCodes[] = { '1', '2', '3', '4', 'q', 'w', 'e', 'r',
                                         'Q', 'W', 'E', 'R' };
        for (int code : keyCodes)
            if (!juce::KeyPress::isKeyCurrentlyDown(code))
                processor.getKeyboardMapper().handleKeyUp(code);
    }
    return false;
}

void BegoniaEditor::timerCallback()
{
    // Chord button visual feedback
    chordPanel.refresh();

    // Keyboard display
    keyboardDisplay.setRootNote(processor.getLastRootNote());
    keyboardDisplay.setChordName(processor.getLastChordName());
    keyboardDisplay.setChordNotes(processor.getChordDisplayState().notesBitmask);

    // Circle of fifths display
    auto ds = processor.getChordDisplayState();
    if (ds.rootPitchClass >= 0)
        circleDisplay.setCurrentChord(ds.rootPitchClass, ds.chordType, ds.notesBitmask);
    else
        circleDisplay.clearChord();

    // Re-grab keyboard focus when mouse re-enters plugin window
    if (isVisible() && !hasKeyboardFocus(true) && isMouseOverOrDragging(true))
        grabKeyboardFocus();
}
