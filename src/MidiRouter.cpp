#include "MidiRouter.h"
#include <algorithm>

void MidiRouter::routeChord(juce::MidiBuffer& outBuffer,
                             const std::vector<int>& prevNotes,
                             const std::vector<int>& newNotes,
                             int bassNote,
                             const Config& config,
                             int samplePosition) const
{
    if (!config.midiOutEnabled)
        return;

    const int chordCh = juce::jlimit(1, 16, config.chordChannel) - 1;  // 0-indexed
    const int bassCh  = juce::jlimit(1, 16, config.bassChannel)  - 1;
    const int vel     = static_cast<int>(config.velocity * 127.0f);
    const int velClamped = juce::jlimit(1, 127, vel);

    // Determine previous bass note (lowest note in prevNotes when bass is on its own channel)
    const bool separateBassCh = config.bassEnabled && chordCh != bassCh;
    int prevBassNote = -1;
    if (separateBassCh && !prevNotes.empty())
        prevBassNote = *std::min_element(prevNotes.begin(), prevNotes.end());

    // Note-off for all previous chord notes (excluding bass if it's on a separate channel)
    for (int note : prevNotes)
    {
        if (note >= 0 && note <= 127)
        {
            if (separateBassCh && note == prevBassNote)
                continue;
            outBuffer.addEvent(juce::MidiMessage::noteOff(chordCh + 1, note, 0.0f),
                               samplePosition);
        }
    }

    // Note-off for previous bass note on bass channel
    if (separateBassCh && prevBassNote >= 0)
        outBuffer.addEvent(juce::MidiMessage::noteOff(bassCh + 1, prevBassNote, 0.0f),
                           samplePosition);

    // Note-on for new chord notes (excluding bass if it's on a separate channel)
    for (int note : newNotes)
    {
        if (note >= 0 && note <= 127)
        {
            if (separateBassCh && note == bassNote)
                continue;
            outBuffer.addEvent(juce::MidiMessage::noteOn(chordCh + 1, note, (juce::uint8)velClamped),
                               samplePosition);
        }
    }

    // Bass note on its own channel (same-channel case is already covered by the loop above)
    if (separateBassCh && bassNote >= 0 && bassNote <= 127)
        outBuffer.addEvent(juce::MidiMessage::noteOn(bassCh + 1, bassNote, (juce::uint8)velClamped),
                           samplePosition);
}

void MidiRouter::allNotesOff(juce::MidiBuffer& outBuffer,
                              const std::vector<int>& activeNotes,
                              int activeBassNote,
                              const Config& config,
                              int samplePosition) const
{
    if (!config.midiOutEnabled)
        return;

    const int chordCh = juce::jlimit(1, 16, config.chordChannel) - 1;
    const int bassCh  = juce::jlimit(1, 16, config.bassChannel)  - 1;

    for (int note : activeNotes)
    {
        if (note >= 0 && note <= 127)
            outBuffer.addEvent(juce::MidiMessage::noteOff(chordCh + 1, note, 0.0f),
                               samplePosition);
    }

    if (config.bassEnabled && activeBassNote >= 0 && chordCh != bassCh)
    {
        outBuffer.addEvent(juce::MidiMessage::noteOff(bassCh + 1, activeBassNote, 0.0f),
                           samplePosition);
    }
}
