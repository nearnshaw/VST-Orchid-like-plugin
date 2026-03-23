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

    // Note-off for all previous notes
    for (int note : prevNotes)
    {
        if (note >= 0 && note <= 127)
            outBuffer.addEvent(juce::MidiMessage::noteOff(chordCh + 1, note, 0.0f),
                               samplePosition);
    }

    // Note-off for previous bass note (if separate channel)
    // We track the previous bass note via prevNotes[0] (lowest), but the caller manages that.
    // The bass note-off is handled implicitly: we send note-off for ALL prevNotes on chord channel,
    // and we'll send bass note-off by checking if bass channel differs.
    if (config.bassEnabled && chordCh != bassCh && !prevNotes.empty())
    {
        // Find the previous bass note (minimum of prevNotes)
        int prevBass = *std::min_element(prevNotes.begin(), prevNotes.end());
        outBuffer.addEvent(juce::MidiMessage::noteOff(bassCh + 1, prevBass, 0.0f),
                           samplePosition);
    }

    // Note-on for new chord notes
    for (int note : newNotes)
    {
        if (note >= 0 && note <= 127)
            outBuffer.addEvent(juce::MidiMessage::noteOn(chordCh + 1, note, (juce::uint8)velClamped),
                               samplePosition);
    }

    // Bass note on separate channel
    if (config.bassEnabled && bassNote >= 0 && bassNote <= 127)
    {
        if (chordCh != bassCh)
        {
            outBuffer.addEvent(juce::MidiMessage::noteOn(bassCh + 1, bassNote, (juce::uint8)velClamped),
                               samplePosition);
        }
    }
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
