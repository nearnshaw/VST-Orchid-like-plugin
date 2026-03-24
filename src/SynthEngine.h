#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "SynthVoice.h"
#include <vector>

class SynthEngine
{
public:
    static constexpr int kNumVoices = 16;

    SynthEngine();

    void prepare  (double sampleRate, int samplesPerBlock, int numChannels);
    void reset    ();

    void setEngine  (BegoniaVoice::Engine e);
    void setADSR    (float attack, float decay, float sustain, float release);
    void setFilter  (float cutoffHz, float resonance);
    void setGain    (float gainLinear);

    // Play a set of MIDI notes as a chord (stops previous chord first)
    void playChord      (const std::vector<int>& midiNotes, float velocity);
    void releaseChord   ();
    // Release every voice regardless of activeNotes tracking (use in arp/strum mode)
    void releaseAllNotes();

    // Play / release a single note (for bass engine and arp/strum)
    void playNote   (int midiNote, float velocity);
    void releaseNote(int midiNote);

    // Called from processBlock to render audio
    void renderNextBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

private:
    juce::Synthesiser synth;

    double currentSampleRate   = 44100.0;
    int    currentBlockSize    = 512;
    int    currentNumChannels  = 2;

    BegoniaVoice::Engine currentEngine = BegoniaVoice::Engine::Piano;
    float currentAttack    = 0.01f;
    float currentDecay     = 0.3f;
    float currentSustain   = 0.7f;
    float currentRelease   = 0.5f;
    float currentCutoff    = 8000.0f;
    float currentResonance = 0.7f;

    // Global effects (applied post-synth)
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb        reverb;
    juce::dsp::Gain<float>   outputGain;

    bool chorusEnabled = false;
    bool reverbEnabled = false;

    std::vector<int> activeNotes;

    void applyEngineEffects();
    void updateAllVoices();
};
