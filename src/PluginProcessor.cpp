#include "PluginProcessor.h"
#include "PluginEditor.h"

BegoniaProcessor::BegoniaProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "BegoniaState", createParameterLayout())
{
}

BegoniaProcessor::~BegoniaProcessor() = default;

bool BegoniaProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo output only; no audio input needed (MIDI instrument)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void BegoniaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    cacheParameterPointers();

    synthEngine.prepare(sampleRate, samplesPerBlock, 2);

    // Force initial sync of all parameters
    prevSynthEngine = -1;
    prevAttack = prevDecay = prevSustain = prevRelease = -1.0f;
    prevCutoff = prevResonance = prevGain = -1.0f;
    updateSynthParameters();
}

void BegoniaProcessor::releaseResources()
{
    synthEngine.reset();
    currentNotes.clear();
    currentRootNote  = -1;
    currentBassNote  = -1;
}

// Returns the diatonic chord type for a pitch class given a key and scale.
// Returns ChordEngine::None if the note is not diatonic (non-standard triad degree).
static uint8_t getDiatonicChordType(int pc, int K, int scale)
{
    static const int kMajOffs[13][4] = {
        { 0, 5, 7,-1}, // Major
        { 3, 8,10,-1}, // Natural Minor
        { 3, 5,10,-1}, // Dorian
        { 1, 3, 8,-1}, // Phrygian
        { 0, 2, 7,-1}, // Lydian
        { 0, 5,10,-1}, // Mixolydian
        { 1, 6, 8,-1}, // Locrian
        { 7, 8,-1,-1}, // Harmonic Minor
        { 5, 7,-1,-1}, // Melodic Minor
        { 0, 2,-1,-1}, // Lydian Dominant
        { 0, 1,-1,-1}, // Phrygian Dominant
        { 7, 8,-1,-1}, // Hungarian Minor
        { 0,10,-1,-1}, // Mixolydian b6
    };
    static const int kMinOffs[13][4] = {
        { 2, 4, 9,-1}, // Major
        { 0, 5, 7,-1}, // Natural Minor
        { 0, 2, 7,-1}, // Dorian
        { 0, 5,10,-1}, // Phrygian
        { 4, 9,11,-1}, // Lydian
        { 2, 7, 9,-1}, // Mixolydian
        { 3, 5,10,-1}, // Locrian
        { 0, 5,-1,-1}, // Harmonic Minor
        { 0, 2,-1,-1}, // Melodic Minor
        { 7, 9,-1,-1}, // Lydian Dominant
        { 5,10,-1,-1}, // Phrygian Dominant
        { 0,11,-1,-1}, // Hungarian Minor
        { 5, 7,-1,-1}, // Mixolydian b6
    };
    static const int kDimOffs[13][4] = {
        {11,-1,-1,-1}, // Major
        { 2,-1,-1,-1}, // Natural Minor
        { 9,-1,-1,-1}, // Dorian
        { 7,-1,-1,-1}, // Phrygian
        { 6,-1,-1,-1}, // Lydian
        { 4,-1,-1,-1}, // Mixolydian
        { 0,-1,-1,-1}, // Locrian
        { 2,11,-1,-1}, // Harmonic Minor
        { 9,11,-1,-1}, // Melodic Minor
        { 4, 6,-1,-1}, // Lydian Dominant
        { 4, 7,-1,-1}, // Phrygian Dominant
        { 6,-1,-1,-1}, // Hungarian Minor
        { 2, 4,-1,-1}, // Mixolydian b6
    };

    const int s = juce::jlimit(0, 12, scale);
    for (int j = 0; j < 4 && kMajOffs[s][j] >= 0; ++j)
        if (pc == (K + kMajOffs[s][j]) % 12) return ChordEngine::Major;
    for (int j = 0; j < 4 && kMinOffs[s][j] >= 0; ++j)
        if (pc == (K + kMinOffs[s][j]) % 12) return ChordEngine::Minor;
    for (int j = 0; j < 4 && kDimOffs[s][j] >= 0; ++j)
        if (pc == (K + kDimOffs[s][j]) % 12) return ChordEngine::Dim;
    return ChordEngine::None;
}

void BegoniaProcessor::processBlock(juce::AudioBuffer<float>& buffer,
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
    uint8_t chordType  = chordState.chordType;
    const uint8_t extensions = chordState.extensions;

    // Key Mode: auto-determine chord type from root note + selected key/scale
    if (pKeyMode && pKeyMode->get() && currentRootNote >= 0)
    {
        const int pc    = currentRootNote % 12;
        const int key   = pSelectedKey   ? pSelectedKey->get()   : 0;
        const int scale = pSelectedScale ? pSelectedScale->get() : 0;
        chordType = getDiatonicChordType(pc, key, scale);
    }

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
                displayNotesMask.store(0,  std::memory_order_release);
                displayRootPC.store   (-1, std::memory_order_release);
                displayChordType.store(0,  std::memory_order_release);
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

    // Determine desired notes:
    //   - root held + chord keys → full voiced chord
    //   - root held + no chord keys → single passthrough note
    //   - no root → silence
    if (currentRootNote >= 0)
    {
        std::vector<int> newNotes;
        int newBass = -1;

        if (chordType != 0 || extensions != 0)
        {
            auto voicingParams = buildVoicingParams();
            auto intervals     = chordEngine.buildIntervals(chordType, extensions);
            newNotes           = voicingEngine.applyVoicing(intervals, currentRootNote, voicingParams);
            newBass            = voicingEngine.getBassNote(newNotes);
        }
        else
        {
            // No chord keys held — pass the raw MIDI note straight through
            newNotes = { currentRootNote };
        }

        if (newNotes != currentNotes)
        {
            auto config     = buildRouterConfig();
            config.velocity = 0.8f;

            midiRouter.routeChord(outMidi, currentNotes, newNotes,
                                  newBass, config, 0);
            synthEngine.playChord(newNotes, config.velocity);

            currentNotes    = newNotes;
            currentBassNote = newBass;
            lastChordName   = (chordType != 0 || extensions != 0)
                                  ? ChordEngine::getChordName(currentRootNote, chordType, extensions)
                                  : "";

            uint16_t mask = 0;
            for (int note : currentNotes)
                mask |= uint16_t(1) << (note % 12);
            displayNotesMask.store(mask,                          std::memory_order_release);
            displayRootPC.store   (int8_t(currentRootNote % 12), std::memory_order_release);
            displayChordType.store(chordType,                     std::memory_order_release);
        }
    }
    else if (!currentNotes.empty())
    {
        // No root note — silence everything
        auto config = buildRouterConfig();
        midiRouter.allNotesOff(outMidi, currentNotes, currentBassNote, config, 0);
        synthEngine.releaseChord();
        currentNotes.clear();
        currentBassNote = -1;
        lastChordName   = "";
        displayNotesMask.store(0,  std::memory_order_release);
        displayRootPC.store   (-1, std::memory_order_release);
        displayChordType.store(0,  std::memory_order_release);
    }

    // Render synth audio (only if synth is enabled)
    if (!pSynthEnabled || pSynthEnabled->get())
    {
        juce::MidiBuffer emptyMidi;
        synthEngine.renderNextBlock(buffer, emptyMidi);
    }

    // Replace incoming MIDI with our output
    midiBuffer.swapWith(outMidi);
}

void BegoniaProcessor::cacheParameterPointers()
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
    pSynthEnabled    = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::SynthEnabled));
    pKeyMode         = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::KeyMode));
    pSelectedKey     = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::SelectedKey));
    pSelectedScale   = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::SelectedScale));
}

void BegoniaProcessor::updateSynthParameters()
{
    if (!pSynthEngine) return;

    int engineIdx = pSynthEngine->get();
    if (engineIdx != prevSynthEngine)
    {
        prevSynthEngine = engineIdx;
        synthEngine.setEngine(static_cast<BegoniaVoice::Engine>(engineIdx));
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

MidiRouter::Config BegoniaProcessor::buildRouterConfig() const
{
    MidiRouter::Config config;
    if (pChordChannel)   config.chordChannel    = pChordChannel->get();
    if (pBassChannel)    config.bassChannel     = pBassChannel->get();
    if (pBassEnabled)    config.bassEnabled     = pBassEnabled->get();
    if (pMidiOutEnabled) config.midiOutEnabled  = pMidiOutEnabled->get();
    config.velocity = 0.8f;
    return config;
}

VoicingEngine::VoicingParams BegoniaProcessor::buildVoicingParams() const
{
    VoicingEngine::VoicingParams params;
    if (pVoicingType)  params.voicing      = static_cast<VoicingEngine::Voicing>(pVoicingType->get());
    if (pOctaveOffset) params.octaveOffset = pOctaveOffset->get();
    if (pBassEnabled)  params.bassEnabled  = pBassEnabled->get();
    return params;
}

void BegoniaProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}

void BegoniaProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* BegoniaProcessor::createEditor()
{
    return new BegoniaEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BegoniaProcessor();
}
