#include "SynthEngine.h"

SynthEngine::SynthEngine()
{
    synth.addSound(new OrchidSound());
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice(new OrchidVoice());

    // Reverb default settings
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = 0.5f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.25f;
    reverbParams.dryLevel   = 0.75f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate  = sampleRate;
    currentBlockSize   = samplesPerBlock;
    currentNumChannels = numChannels;

    synth.setCurrentPlaybackSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32)samplesPerBlock,
                                  (juce::uint32)numChannels };

    chorus.prepare(spec);
    chorus.setRate(0.3f);
    chorus.setDepth(0.005f);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.1f);
    chorus.setMix(0.4f);

    reverb.prepare(spec);
    outputGain.prepare(spec);
    outputGain.setGainLinear(0.8f);

    // Prepare each voice
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<OrchidVoice*>(synth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock, numChannels);
    }
}

void SynthEngine::reset()
{
    synth.allNotesOff(0, false);
    activeNotes.clear();
}

void SynthEngine::setEngine(OrchidVoice::Engine e)
{
    currentEngine = e;
    updateAllVoices();
    applyEngineEffects();
}

void SynthEngine::setADSR(float attack, float decay, float sustain, float release)
{
    currentAttack  = attack;
    currentDecay   = decay;
    currentSustain = sustain;
    currentRelease = release;
    updateAllVoices();
}

void SynthEngine::setFilter(float cutoffHz, float resonance)
{
    currentCutoff    = cutoffHz;
    currentResonance = resonance;
    updateAllVoices();
}

void SynthEngine::setGain(float gainLinear)
{
    outputGain.setGainLinear(gainLinear);
}

void SynthEngine::playChord(const std::vector<int>& midiNotes, float velocity)
{
    // Release previous chord
    releaseChord();

    const int velInt = static_cast<int>(velocity * 127.0f);
    const int velClamped = juce::jlimit(1, 127, velInt);

    juce::MidiBuffer buf;
    for (int note : midiNotes)
    {
        if (note >= 0 && note <= 127)
            buf.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)velClamped), 0);
    }

    // Render 0 samples just to process the MIDI events
    juce::AudioBuffer<float> dummy(currentNumChannels, 0);
    synth.renderNextBlock(dummy, buf, 0, 0);

    activeNotes = midiNotes;
}

void SynthEngine::releaseChord()
{
    if (activeNotes.empty())
        return;

    juce::MidiBuffer buf;
    for (int note : activeNotes)
    {
        if (note >= 0 && note <= 127)
            buf.addEvent(juce::MidiMessage::noteOff(1, note, 0.0f), 0);
    }

    juce::AudioBuffer<float> dummy(currentNumChannels, 0);
    synth.renderNextBlock(dummy, buf, 0, 0);

    activeNotes.clear();
}

void SynthEngine::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    // Render voices
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply effects based on engine
    if (chorusEnabled || reverbEnabled)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);

        if (chorusEnabled)
            chorus.process(ctx);
        if (reverbEnabled)
            reverb.process(ctx);
    }

    // Apply output gain
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        outputGain.process(ctx);
    }
}

void SynthEngine::applyEngineEffects()
{
    switch (currentEngine)
    {
        case OrchidVoice::Engine::Piano:
            chorusEnabled = false;
            reverbEnabled = false;
            break;
        case OrchidVoice::Engine::Pad:
            chorusEnabled = true;
            reverbEnabled = true;
            break;
        case OrchidVoice::Engine::Synth:
            chorusEnabled = false;
            reverbEnabled = true;
            break;
    }
}

void SynthEngine::updateAllVoices()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<OrchidVoice*>(synth.getVoice(i)))
        {
            voice->setEngine(currentEngine);
            voice->setADSR(currentAttack, currentDecay, currentSustain, currentRelease);
            voice->setFilter(currentCutoff, currentResonance, currentSampleRate);
        }
    }
}
