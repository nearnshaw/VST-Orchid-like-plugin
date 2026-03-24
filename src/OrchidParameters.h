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
    inline constexpr auto SelectedScale    = "selectedScale";
    inline constexpr auto SynthEnabled     = "synthEnabled";
    inline constexpr auto KeyMode          = "keyMode";

    // Bass engine
    inline constexpr auto BassEngineType  = "bassEngineType";

    // Performance mode
    inline constexpr auto PerfMode        = "perfMode";
    inline constexpr auto StrumSpeed      = "strumSpeed";
    inline constexpr auto StrumUp         = "strumUp";
    inline constexpr auto ArpPattern      = "arpPattern";
    inline constexpr auto ArpBPM          = "arpBPM";
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

    // Synth engine: 0=Piano, 1=Pad, 2=Synth, 3=Strings, 4=Pluck, 5=Organ
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::SynthEngineType, "Synth Engine", 0, 5, 0));

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

    // Synth audio enabled (false = MIDI out only, no audio rendering)
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::SynthEnabled, "Synth Audio", true));

    // Selected key for circle of fifths diatonic highlighting (0=C … 11=B)
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::SelectedKey, "Key", 0, 11, 0));

    // Selected scale/mode (0=Major … 12=Mixolydian b6)
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::SelectedScale, "Scale", 0, 12, 0));

    // Key Mode: auto-determine chord type from root + selected key/scale
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::KeyMode, "Key Mode", false));

    // Bass engine: 0=Piano, 1=Pad, 2=Synth, 3=Strings, 4=Pluck, 5=Organ
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::BassEngineType, "Bass Engine", 0, 5, 3));

    // Performance mode: 0=Normal, 1=Strum, 2=Arpeggiator
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::PerfMode, "Perf Mode", 0, 2, 0));

    // Strum speed: 0=Slow, 1=Medium, 2=Fast
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::StrumSpeed, "Strum Speed", 0, 2, 1));

    // Strum direction: true=Up (low→high), false=Down (high→low)
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::StrumUp, "Strum Up", true));

    // Arp pattern: 0=Up, 1=Down, 2=UpDown, 3=DownUp, 4=Random
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::ArpPattern, "Arp Pattern", 0, 4, 0));

    // Arp BPM: song tempo used to derive 1/16 note step interval
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ArpBPM, "Arp BPM",
        NormalisableRange<float>(40.0f, 300.0f), 100.0f));

    return { params.begin(), params.end() };
}
