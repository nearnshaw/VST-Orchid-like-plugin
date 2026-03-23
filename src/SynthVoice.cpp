#include "SynthVoice.h"

OrchidVoice::OrchidVoice()
{
    // Default ADSR (Piano-like)
    adsrParams.attack  = 0.01f;
    adsrParams.decay   = 0.3f;
    adsrParams.sustain = 0.7f;
    adsrParams.release = 0.5f;
    adsr.setParameters(adsrParams);

    configureOscillators();
}

void OrchidVoice::setEngine(Engine e)
{
    currentEngine = e;
    configureOscillators();
    applyEngineDefaults();
}

void OrchidVoice::setADSR(float attack, float decay, float sustain, float release)
{
    adsrParams.attack  = attack;
    adsrParams.decay   = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    adsr.setParameters(adsrParams);
}

void OrchidVoice::setFilter(float cutoffHz, float resonance, double sr)
{
    filter.setCutoffFrequency(cutoffHz);
    filter.setResonance(resonance);
}

void OrchidVoice::setSampleRate(double sr)
{
    sampleRate = sr;
}

void OrchidVoice::prepareToPlay(double sr, int samplesPerBlock, int numChannels)
{
    sampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };

    osc1.prepare(spec);
    osc2.prepare(spec);
    oscSub.prepare(spec);
    filter.prepare(spec);
    adsr.setSampleRate(sr);
    gainSmoother.reset(sr, 0.005);

    configureOscillators();
}

bool OrchidVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<OrchidSound*>(sound) != nullptr;
}

void OrchidVoice::configureOscillators()
{
    switch (currentEngine)
    {
        case Engine::Piano:
            // Sine with harmonic content
            osc1.initialise([](float x) {
                return std::sin(x) + 0.3f * std::sin(2.0f * x) + 0.08f * std::sin(3.0f * x);
            }, 512);
            osc2.initialise([](float x) { return 0.0f; }, 512); // Silent
            oscSub.initialise([](float x) { return 0.0f; }, 512);
            break;

        case Engine::Pad:
            // Pure sine (detuning handled by osc2)
            osc1.initialise([](float x) { return std::sin(x); }, 512);
            osc2.initialise([](float x) { return std::sin(x); }, 512); // Detuned copy
            oscSub.initialise([](float x) { return std::sin(x) * 0.3f; }, 512); // Sub
            break;

        case Engine::Synth:
            // Bandlimited sawtooth approximation via juce built-in
            osc1.initialise([](float x) {
                // Simplified sawtooth: linear ramp from -1 to 1
                return (x / juce::MathConstants<float>::pi);
            }, 1024);
            osc2.initialise([](float x) {
                return (x / juce::MathConstants<float>::pi) * 0.5f;
            }, 1024);
            oscSub.initialise([](float x) { return std::sin(x) * 0.4f; }, 512);
            break;
    }
}

void OrchidVoice::applyEngineDefaults()
{
    // ADSR defaults per engine — only applied if user hasn't customised
    // (these are suggestions; actual ADSR is set by setADSR calls from processor)
    switch (currentEngine)
    {
        case Engine::Piano:
            setADSR(0.005f, 0.5f, 0.4f, 0.3f);
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filter.setCutoffFrequency(12000.0f);
            filter.setResonance(0.5f);
            break;

        case Engine::Pad:
            setADSR(0.4f, 0.8f, 0.8f, 1.5f);
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filter.setCutoffFrequency(2000.0f);
            filter.setResonance(0.3f);
            break;

        case Engine::Synth:
            setADSR(0.02f, 0.4f, 0.6f, 0.4f);
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filter.setCutoffFrequency(4000.0f);
            filter.setResonance(1.5f);
            break;
    }
}

void OrchidVoice::startNote(int midiNoteNumber, float velocity,
                              juce::SynthesiserSound*, int)
{
    noteFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    noteVelocity  = velocity;
    isPlaying     = true;

    osc1.setFrequency(noteFrequency);

    // Slight detune for Pad (±5 cents) and Synth (±8 cents)
    float detuneRatio = 1.0f;
    if (currentEngine == Engine::Pad)   detuneRatio = 1.003f;  // ~5 cents
    if (currentEngine == Engine::Synth) detuneRatio = 1.005f;  // ~8 cents
    osc2.setFrequency(noteFrequency * detuneRatio);

    // Sub oscillator one octave below
    oscSub.setFrequency(noteFrequency * 0.5f);

    // Velocity-sensitive filter opening for Synth engine
    if (currentEngine == Engine::Synth)
    {
        float velCutoff = 500.0f + velocity * 6000.0f;
        filter.setCutoffFrequency(velCutoff);
    }

    adsr.noteOn();
    gainSmoother.setTargetValue(velocity * 0.6f);
}

void OrchidVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
        isPlaying = false;
    }
}

void OrchidVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                   int startSample, int numSamples)
{
    if (!isPlaying && !adsr.isActive())
        return;

    // Create a temporary mono buffer for this voice
    juce::AudioBuffer<float> tempBuf(1, numSamples);
    tempBuf.clear();

    auto* dest = tempBuf.getWritePointer(0);

    // Render samples
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = osc1.processSample(0.0f);

        if (currentEngine == Engine::Pad || currentEngine == Engine::Synth)
            sample = (sample + osc2.processSample(0.0f)) * 0.5f;

        if (currentEngine == Engine::Pad)
            sample += oscSub.processSample(0.0f) * 0.2f;
        if (currentEngine == Engine::Synth)
            sample += oscSub.processSample(0.0f) * 0.15f;

        dest[i] = sample * gainSmoother.getNextValue();
    }

    // Apply ADSR
    adsr.applyEnvelopeToBuffer(tempBuf, 0, numSamples);

    // Apply filter
    juce::dsp::AudioBlock<float> block(tempBuf);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    filter.process(ctx);

    // Mix into output buffer (all channels)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom(ch, startSample, tempBuf, 0, 0, numSamples);

    // Check if voice finished
    if (!adsr.isActive())
    {
        clearCurrentNote();
        isPlaying = false;
    }
}
