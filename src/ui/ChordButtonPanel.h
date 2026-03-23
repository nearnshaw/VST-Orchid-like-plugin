#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ChordEngine.h"
#include "../KeyboardMapper.h"

// Displays the 4 chord type buttons (1/2/3/4) and 4 extension buttons (Q/W/E/R).
// Buttons are visual feedback: they highlight when the corresponding key is held.
// Clicking a button also fires the chord state (mouse input alternative to keyboard).
class ChordButtonPanel : public juce::Component
{
public:
    explicit ChordButtonPanel(KeyboardMapper& km);

    void paint   (juce::Graphics&) override;
    void resized () override;

    // Call at ~30Hz to refresh visual state from KeyboardMapper
    void refresh();

private:
    KeyboardMapper& keyboardMapper;

    struct ChordButton
    {
        juce::String label;
        juce::String keyHint;
        bool isExtension = false;
        uint8_t bit = 0;
        bool isActive = false;
        juce::Rectangle<int> bounds;
    };

    std::array<ChordButton, 8> buttons;

    void layoutButtons();
    void paintButton (juce::Graphics& g, const ChordButton& btn) const;
    void mouseDown   (const juce::MouseEvent& e) override;
    void mouseUp     (const juce::MouseEvent& e) override;

    ChordButton* hitTest (juce::Point<int> pt);

    static constexpr float kCornerRadius = 6.0f;

    // Colour palette
    static const juce::Colour kBgColour;
    static const juce::Colour kButtonColour;
    static const juce::Colour kActiveChordColour;
    static const juce::Colour kActiveExtColour;
    static const juce::Colour kTextColour;
};
