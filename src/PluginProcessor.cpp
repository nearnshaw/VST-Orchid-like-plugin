#include "PluginProcessor.h"
#include "PluginEditor.h"

OrchidProcessor::OrchidProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "OrchidState", createParameterLayout())
{
}

OrchidProcessor::~OrchidProcessor() = default;

bool OrchidProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo output only; no audio input needed (MIDI instrument)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void OrchidProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    cacheParameterPointers();

    synthEngine.prepare(sampleRate, samplesPerBlock, 2);

    // Force initial sync of all parameters
    prevSynthEngine = -1;
    prevAttack = prevDecay = prevSustain = prevRelease = -1.0f;
    prevCutoff = prevResonance = prevGain = -1.0f;
    updateSynthParameters();
}

void OrchidProcessor::releaseResources()
{
    synthEngine.reset();
    currentNotes.clear();
    currentRootNote  = -1;
    currentBassNote  = -1;
}

void OrchidProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Update synth parameters if they changed
    updateSynthParameters();

    // Global key monitor toggle
    if (pGlobalKeyMon)
    {
        bool wantGlobal = pGlobalKeyMon->get();
        if (wantGlobal != prevGlobalKeyMon)
        {
            prevGlobalKeyMon = wantGlobal;
            // Must be done on message thread — schedule it
            juce::MessageManager::callAsync([this, wantGlobal]() {
                keyboardMapper.setGlobalMonitorEnabled(wantGlobal);
            });
        }
    }

    // Build output MIDI buffer
    juce::MidiBuffer outMidi;

    // Get current keyboard/chord state (atomic read — safe on audio thread)
    auto chordState = keyboardMapper.getCurrentState();
    const uint8_t chordType  = chordState.chordType;
    const uint8_t extensions = chordState.extensions;

    // Process incoming MIDI to extract root note
    for (const auto metadata : midiBuffer)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            currentRootNote = msg.getNoteNumber();
            lastRootNote.store(currentRootNote, std::memory_order_release);
        }
        else if (msg.isNoteOff())
        {
            if (msg.getNoteNumber() == currentRootNote)
            {
                // Release chord
                auto config = buildRouterConfig();
                midiRouter.allNotesOff(outMidi, currentNotes, currentBassNote,
                                       config, metadata.samplePosition);
                synthEngine.releaseChord();
                currentNotes.clear();
                currentRootNote = -1;
                currentBassNote = -1;
                lastRootNote.store(-1, std::memory_order_release);
                lastChordName = "";
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            auto config = buildRouterConfig();
            midiRouter.allNotesOff(outMidi, currentNotes, currentBassNote, config, 0);
            synthEngine.reset();
            currentNotes.clear();
            currentRootNote = -1;
            currentBassNote = -1;
        }
    }

    // Generate chord if we have a root note and at least one key held (type or extension)
    if ((chordType != 0 || extensions != 0) && currentRootNote >= 0)
    {
        auto voicingParams = buildVoicingParams();
        auto intervals     = chordEngine.buildIntervals(chordType, extensions);
        auto newNotes      = voicingEngine.applyVoicing(intervals, currentRootNote, voicingParams);
        int  newBass       = voicingEngine.getBassNote(newNotes);

        if (newNotes != currentNotes)
        {
            auto config = buildRouterConfig();
            config.velocity = 0.8f;

            midiRouter.routeChord(outMidi, currentNotes, newNotes,
                                  newBass, config, 0);
            synthEngine.playChord(newNotes, config.velocity);

            currentNotes    = newNotes;
            currentBassNote = newBass;
            lastChordName   = ChordEngine::getChordName(currentRootNote, chordType, extensions);
        }
    }
    else if (chordType == 0 && extensions == 0 && !currentNotes.empty())
    {
        // Chord type released — stop the chord
        auto config = buildRouterConfig();
        midiRouter.allNotesOff(outMidi, currentNotes, currentBassNote, config, 0);
        synthEngine.releaseChord();
        currentNotes.clear();
        currentBassNote = -1;
        lastChordName = "";
    }

    // Render synth audio
    juce::MidiBuffer emptyMidi;
    synthEngine.renderNextBlock(buffer, emptyMidi);

    // Replace incoming MIDI with our output
    midiBuffer.swapWith(outMidi);
}

void OrchidProcessor::cacheParameterPointers()
{
    pVoicingType     = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::VoicingType));
    pOctaveOffset    = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::OctaveOffset));
    pBassEnabled     = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::BassEnabled));
    pSynthEngine     = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::SynthEngineType));
    pAttack          = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::Attack));
    pDecay           = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::Decay));
    pSustain         = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::Sustain));
    pRelease         = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::Release));
    pFilterCutoff    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::FilterCutoff));
    pFilterResonance = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::FilterResonance));
    pChordChannel    = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::ChordChannel));
    pBassChannel     = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::BassChannel));
    pMidiOutEnabled  = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::MidiOutEnabled));
    pOutputGain      = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::OutputGain));
    pGlobalKeyMon    = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::GlobalKeyMonitor));
}

void OrchidProcessor::updateSynthParameters()
{
    if (!pSynthEngine) return;

    int engineIdx = pSynthEngine->get();
    if (engineIdx != prevSynthEngine)
    {
        prevSynthEngine = engineIdx;
        synthEngine.setEngine(static_cast<OrchidVoice::Engine>(engineIdx));
    }

    float a = pAttack->get(), d = pDecay->get(), s = pSustain->get(), r = pRelease->get();
    if (a != prevAttack || d != prevDecay || s != prevSustain || r != prevRelease)
    {
        prevAttack = a; prevDecay = d; prevSustain = s; prevRelease = r;
        synthEngine.setADSR(a, d, s, r);
    }

    float cutoff = pFilterCutoff->get(), res = pFilterResonance->get();
    if (cutoff != prevCutoff || res != prevResonance)
    {
        prevCutoff = cutoff; prevResonance = res;
        synthEngine.setFilter(cutoff, res);
    }

    float gain = pOutputGain->get();
    if (gain != prevGain)
    {
        prevGain = gain;
        synthEngine.setGain(gain);
    }
}

MidiRouter::Config OrchidProcessor::buildRouterConfig() const
{
    MidiRouter::Config config;
    if (pChordChannel)   config.chordChannel    = pChordChannel->get();
    if (pBassChannel)    config.bassChannel     = pBassChannel->get();
    if (pBassEnabled)    config.bassEnabled     = pBassEnabled->get();
    if (pMidiOutEnabled) config.midiOutEnabled  = pMidiOutEnabled->get();
    config.velocity = 0.8f;
    return config;
}

VoicingEngine::VoicingParams OrchidProcessor::buildVoicingParams() const
{
    VoicingEngine::VoicingParams params;
    if (pVoicingType)  params.voicing      = static_cast<VoicingEngine::Voicing>(pVoicingType->get());
    if (pOctaveOffset) params.octaveOffset = pOctaveOffset->get();
    if (pBassEnabled)  params.bassEnabled  = pBassEnabled->get();
    return params;
}

void OrchidProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}

void OrchidProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* OrchidProcessor::createEditor()
{
    return new OrchidEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchidProcessor();
}
