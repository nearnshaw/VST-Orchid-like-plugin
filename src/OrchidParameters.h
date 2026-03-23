#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr auto VoicingType      = "voicingType";
    inline constexpr auto OctaveOffset     = "octaveOffset";
    inline constexpr auto BassEnabled      = "bassEnabled";
    inline constexpr auto SynthEngineType  = "synthEngineType";
    inline constexpr auto Attack           = "attack";
    inline constexpr auto Decay            = "decay";
    inline constexpr auto Sustain          = "sustain";
    inline constexpr auto Release          = "release";
    inline constexpr auto FilterCutoff     = "filterCutoff";
    inline constexpr auto FilterResonance  = "filterResonance";
    inline constexpr auto ChordChannel     = "chordChannel";
    inline constexpr auto BassChannel      = "bassChannel";
    inline constexpr auto MidiOutEnabled   = "midiOutEnabled";
    inline constexpr auto OutputGain       = "outputGain";
    inline constexpr auto GlobalKeyMonitor = "globalKeyMonitor";
    inline constexpr auto SelectedKey      = "selectedKey";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Voicing type: 0=Close, 1=Inv1, 2=Inv2, 3=Inv3, 4=Open, 5=Drop2, 6=Spread
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::VoicingType, "Voicing", 0, 6, 0));

    // Octave offset: -2 to +2
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::OctaveOffset, "Octave", -2, 2, 0));

    // Bass note on separate channel
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::BassEnabled, "Bass Enabled", true));

    // Synth engine: 0=Piano, 1=Pad, 2=Synth
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::SynthEngineType, "Synth Engine", 0, 2, 0));

    // ADSR
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::Attack, "Attack",
        NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.01f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::Decay, "Decay",
        NormalisableRange<float>(0.001f, 3.0f, 0.0f, 0.3f), 0.3f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::Sustain, "Sustain",
        NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::Release, "Release",
        NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.5f));

    // Filter
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::FilterCutoff, "Filter Cutoff",
        NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 8000.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::FilterResonance, "Filter Resonance",
        NormalisableRange<float>(0.1f, 10.0f), 0.7f));

    // MIDI routing
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::ChordChannel, "Chord MIDI Ch", 1, 16, 1));

    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::BassChannel, "Bass MIDI Ch", 1, 16, 2));

    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::MidiOutEnabled, "MIDI Out", true));

    // Output gain
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::OutputGain, "Output Gain",
        NormalisableRange<float>(0.0f, 1.5f), 0.8f));

    // Global key monitor (macOS)
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::GlobalKeyMonitor, "Global Key Monitor", false));

    // Selected key for circle of fifths diatonic highlighting (0=C … 11=B)
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::SelectedKey, "Key", 0, 11, 0));

    return { params.begin(), params.end() };
}
