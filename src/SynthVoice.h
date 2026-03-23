#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class OrchidSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class OrchidVoice : public juce::SynthesiserVoice
{
public:
    enum class Engine { Piano = 0, Pad = 1, Synth = 2 };

    OrchidVoice();

    void setEngine  (Engine e);
    void setADSR    (float attack, float decay, float sustain, float release);
    void setFilter  (float cutoffHz, float resonance, double sampleRate);
    void setSampleRate (double sr);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote    (int midiNoteNumber, float velocity,
                       juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote     (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override {}
    void controllerMoved (int controllerNumber, int newControllerValue) override {}
    void renderNextBlock (juce::AudioBuffer<float>& buffer,
                          int startSample, int numSamples) override;
    void prepareToPlay   (double sampleRate, int samplesPerBlock, int numChannels);

private:
    Engine currentEngine = Engine::Piano;
    double sampleRate    = 44100.0;
    float  noteFrequency = 440.0f;
    float  noteVelocity  = 1.0f;

    // Primary oscillator
    juce::dsp::Oscillator<float> osc1;
    // Secondary (detuned) for Pad/Synth
    juce::dsp::Oscillator<float> osc2;
    // Sub oscillator for depth
    juce::dsp::Oscillator<float> oscSub;

    // ADSR
    juce::ADSR           adsr;
    juce::ADSR::Parameters adsrParams;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;

    // Smooth gain for click prevention
    juce::SmoothedValue<float> gainSmoother;

    bool isPlaying = false;

    void configureOscillators();
    void applyEngineDefaults();
};
