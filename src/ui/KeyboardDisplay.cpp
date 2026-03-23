#include "KeyboardDisplay.h"

const juce::Colour KeyboardDisplay::kBgColour   = juce::Colour(0xFF12121E);
const juce::Colour KeyboardDisplay::kWhiteKey   = juce::Colour(0xFFE8E8F0);
const juce::Colour KeyboardDisplay::kBlackKey   = juce::Colour(0xFF1E1E2E);
const juce::Colour KeyboardDisplay::kRootColour = juce::Colour(0xFFE06B3A);
const juce::Colour KeyboardDisplay::kTextColour = juce::Colour(0xFFDDDDEE);

KeyboardDisplay::KeyboardDisplay()
{
}

void KeyboardDisplay::setRootNote(int midiNote)
{
    if (rootNote != midiNote)
    {
        rootNote = midiNote;
        repaint();
    }
}

void KeyboardDisplay::setChordName(const juce::String& name)
{
    if (chordName != name)
    {
        chordName = name;
        repaint();
    }
}

void KeyboardDisplay::resized()
{
    buildKeyLayout();
}

void KeyboardDisplay::buildKeyLayout()
{
    keyRects.clear();

    // Two octaves: C3 (MIDI 48) to B4 (MIDI 71)
    const int startNote = 48;
    const int endNote   = 72;

    // Black key pattern within an octave (relative to C)
    const bool isBlackKey[] = { false, true, false, true, false,
                                 false, true, false, true, false, true, false };

    const float keyAreaHeight = (float)getHeight() - 24.0f; // Leave room for chord name
    const float whiteKeyH = keyAreaHeight;
    const float blackKeyH = keyAreaHeight * 0.62f;

    // Count white keys
    int numWhite = 0;
    for (int n = startNote; n < endNote; ++n)
        if (!isBlackKey[n % 12])
            ++numWhite;

    const float whiteKeyW = (float)getWidth() / (float)numWhite;

    // First pass: place white keys
    int whiteIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (!isBlackKey[n % 12])
        {
            float x = whiteIdx * whiteKeyW;
            keyRects.push_back({ n, { x, 0.0f, whiteKeyW - 1.0f, whiteKeyH }, false });
            ++whiteIdx;
        }
    }

    // Second pass: place black keys (on top)
    whiteIdx = 0;
    for (int n = startNote; n < endNote; ++n)
    {
        if (!isBlackKey[n % 12])
        {
            ++whiteIdx;
        }
        else
        {
            // Black key sits between the previous white key and the next
            float x = (whiteIdx - 1) * whiteKeyW + whiteKeyW * 0.65f;
            float w = whiteKeyW * 0.6f;
            keyRects.push_back({ n, { x, 0.0f, w, blackKeyH }, true });
        }
    }
}

void KeyboardDisplay::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Draw white keys first, then black keys on top
    for (const auto& key : keyRects)
        if (!key.isBlack)
        {
            bool isRoot = (rootNote >= 0 && (key.midiNote % 12) == (rootNote % 12));
            g.setColour(isRoot ? kRootColour : kWhiteKey);
            g.fillRect(key.bounds);
            g.setColour(kBgColour.withAlpha(0.5f));
            g.drawRect(key.bounds, 1.0f);
        }

    for (const auto& key : keyRects)
        if (key.isBlack)
        {
            bool isRoot = (rootNote >= 0 && (key.midiNote % 12) == (rootNote % 12));
            g.setColour(isRoot ? kRootColour.darker(0.3f) : kBlackKey);
            g.fillRect(key.bounds);
        }

    // Chord name display
    const float nameY = (float)getHeight() - 22.0f;
    g.setColour(kBgColour);
    g.fillRect(0.0f, nameY, (float)getWidth(), 22.0f);

    g.setColour(kTextColour);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));

    juce::String display = chordName.isEmpty() ? "—" : chordName;
    g.drawText(display, 0, (int)nameY, getWidth(), 22, juce::Justification::centred);
}
