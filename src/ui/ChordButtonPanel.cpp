#include "ChordButtonPanel.h"

const juce::Colour ChordButtonPanel::kBgColour         = juce::Colour(0xFF1A1A2E);
const juce::Colour ChordButtonPanel::kButtonColour     = juce::Colour(0xFF2D2D44);
const juce::Colour ChordButtonPanel::kActiveChordColour = juce::Colour(0xFFE06B3A); // Orange
const juce::Colour ChordButtonPanel::kActiveExtColour   = juce::Colour(0xFF5E9CF2); // Blue
const juce::Colour ChordButtonPanel::kTextColour        = juce::Colour(0xFFDDDDEE);

ChordButtonPanel::ChordButtonPanel(KeyboardMapper& km) : keyboardMapper(km)
{
    // Chord type buttons
    buttons[0] = { "MAJOR", "[1]", false, ChordEngine::Major, false, {} };
    buttons[1] = { "MINOR", "[2]", false, ChordEngine::Minor, false, {} };
    buttons[2] = { "SUS4",  "[3]", false, ChordEngine::Sus4,  false, {} };
    buttons[3] = { "DIM",   "[4]", false, ChordEngine::Dim,   false, {} };

    // Extension buttons
    buttons[4] = { "6th",  "[Q]", true, ChordEngine::Add6,    false, {} };
    buttons[5] = { "m7",   "[W]", true, ChordEngine::AddM7,   false, {} };
    buttons[6] = { "Maj7", "[E]", true, ChordEngine::AddMaj7, false, {} };
    buttons[7] = { "9th",  "[R]", true, ChordEngine::Add9,    false, {} };
}

void ChordButtonPanel::resized()
{
    layoutButtons();
}

void ChordButtonPanel::layoutButtons()
{
    const int w    = getWidth();
    const int h    = getHeight();
    const int pad  = 6;
    const int btnH = (h - pad * 5) / 4;
    const int btnW = w - pad * 2;

    for (int i = 0; i < 4; ++i)
        buttons[i].bounds = { pad, pad + i * (btnH + pad), btnW, btnH };

    // Extensions below with a small separator
    const int extStartY = pad + 4 * (btnH + pad) + pad;
    const int extBtnH   = (h - extStartY - pad * 5) / 4;

    for (int i = 0; i < 4; ++i)
        buttons[4 + i].bounds = { pad, extStartY + i * (extBtnH + pad), btnW, extBtnH };
}

void ChordButtonPanel::refresh()
{
    auto state = keyboardMapper.getCurrentState();

    for (auto& btn : buttons)
    {
        if (!btn.isExtension)
            btn.isActive = (state.chordType & btn.bit) != 0;
        else
            btn.isActive = (state.extensions & btn.bit) != 0;
    }
    repaint();
}

void ChordButtonPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Section labels
    g.setColour(kTextColour.withAlpha(0.5f));
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

    for (const auto& btn : buttons)
        paintButton(g, btn);
}

void ChordButtonPanel::paintButton(juce::Graphics& g, const ChordButton& btn) const
{
    if (btn.bounds.isEmpty())
        return;

    juce::Colour fill;
    if (btn.isActive)
        fill = btn.isExtension ? kActiveExtColour : kActiveChordColour;
    else
        fill = kButtonColour;

    // Button body
    g.setColour(fill);
    g.fillRoundedRectangle(btn.bounds.toFloat(), kCornerRadius);

    // Border
    g.setColour(btn.isActive ? fill.brighter(0.3f) : kButtonColour.brighter(0.2f));
    g.drawRoundedRectangle(btn.bounds.toFloat(), kCornerRadius, 1.0f);

    // Label
    g.setColour(btn.isActive ? juce::Colours::white : kTextColour);
    g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    g.drawText(btn.label,
               btn.bounds.getX(), btn.bounds.getY(),
               btn.bounds.getWidth() * 2 / 3, btn.bounds.getHeight(),
               juce::Justification::centred);

    // Key hint (right side, smaller)
    g.setColour((btn.isActive ? juce::Colours::white : kTextColour).withAlpha(0.6f));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(btn.keyHint,
               btn.bounds.getRight() - btn.bounds.getWidth() / 3,
               btn.bounds.getY(),
               btn.bounds.getWidth() / 3 - 4, btn.bounds.getHeight(),
               juce::Justification::centredRight);
}

void ChordButtonPanel::mouseDown(const juce::MouseEvent& e)
{
    if (auto* btn = hitTest(e.getPosition()))
    {
        if (!btn->isExtension)
            keyboardMapper.injectChordType(btn->bit, true);
        else
            keyboardMapper.injectExtension(btn->bit, true);
        refresh();
    }
}

void ChordButtonPanel::mouseUp(const juce::MouseEvent& e)
{
    if (auto* btn = hitTest(e.getPosition()))
    {
        if (!btn->isExtension)
            keyboardMapper.injectChordType(btn->bit, false);
        else
            keyboardMapper.injectExtension(btn->bit, false);
        refresh();
    }
}

ChordButtonPanel::ChordButton* ChordButtonPanel::hitTest(juce::Point<int> pt)
{
    for (auto& btn : buttons)
        if (btn.bounds.contains(pt))
            return &btn;
    return nullptr;
}
