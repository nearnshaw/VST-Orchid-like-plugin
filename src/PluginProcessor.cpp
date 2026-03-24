#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <chrono>
#include <random>

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
    currentSampleRate = sampleRate;
    cacheParameterPointers();

    synthEngine.prepare(sampleRate, samplesPerBlock, 2);
    bassEngine.prepare(sampleRate, samplesPerBlock, 2);

    // Force initial sync of all parameters
    prevSynthEngine = -1;
    prevBassEngine  = -1;
    prevAttack = prevDecay = prevSustain = prevRelease = -1.0f;
    prevCutoff = prevResonance = prevGain = -1.0f;
    updateSynthParameters();
}

void BegoniaProcessor::releaseResources()
{
    synthEngine.releaseAllNotes();
    bassEngine.releaseAllNotes();
    currentNotes.clear();
    currentRootNote  = -1;
    currentBassNote  = -1;
    arpSequence.clear();
    arpCurrentNote   = -1;
    arpSampleCounter = 0;
    arpSeqIdx        = 0;
    strumQueue.clear();
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

void BegoniaProcessor::injectMidiNote(int midiNote, bool isDown)
{
    const auto scope = keyEventFifo.write(1);
    if (scope.blockSize1 > 0)
        keyEventBuf[scope.startIndex1] = { midiNote, isDown };
    else if (scope.blockSize2 > 0)
        keyEventBuf[scope.startIndex2] = { midiNote, isDown };
}

void BegoniaProcessor::buildArpSequence()
{
    arpSequence.clear();
    if (arpSequence.capacity() < arpNotes.size() * 2)
        arpSequence.reserve(arpNotes.size() * 2);

    std::vector<int> sorted = arpNotes;
    const int pattern = pArpPattern ? pArpPattern->get() : 0;

    switch (pattern)
    {
        case 0: // Up
            std::sort(sorted.begin(), sorted.end());
            arpSequence = sorted;
            break;
        case 1: // Down
            std::sort(sorted.begin(), sorted.end(), std::greater<int>());
            arpSequence = sorted;
            break;
        case 2: // UpDown
            std::sort(sorted.begin(), sorted.end());
            arpSequence = sorted;
            for (int i = (int)sorted.size() - 2; i >= 1; --i)
                arpSequence.push_back(sorted[i]);
            break;
        case 3: // DownUp
            std::sort(sorted.begin(), sorted.end(), std::greater<int>());
            arpSequence = sorted;
            for (int i = (int)sorted.size() - 2; i >= 1; --i)
                arpSequence.push_back(sorted[i]);
            break;
        case 4: // Random — shuffle once per chord change
        {
            std::sort(sorted.begin(), sorted.end());
            std::mt19937 rng(static_cast<uint32_t>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            std::shuffle(sorted.begin(), sorted.end(), rng);
            arpSequence = sorted;
            break;
        }
        default:
            std::sort(sorted.begin(), sorted.end());
            arpSequence = sorted;
            break;
    }
}

int BegoniaProcessor::computeArpStepSamples() const
{
    const float bpm  = pArpBPM ? pArpBPM->get() : 120.0f;
    // 1/16 note at given BPM
    const float secs = 60.0f / (bpm * 4.0f);
    return juce::jmax(1, (int)(currentSampleRate * secs));
}

int BegoniaProcessor::strumIntervalSamples() const
{
    const int speed = pStrumSpeed ? pStrumSpeed->get() : 1;
    // Slow≈80ms, Medium≈35ms, Fast≈15ms — audibly spread across the chord
    static const float kMs[] = { 80.0f, 35.0f, 15.0f };
    const float ms = kMs[juce::jlimit(0, 2, speed)];
    return juce::jmax(1, (int)(currentSampleRate * ms * 0.001f));
}

void BegoniaProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Drain UI keyboard events into the MIDI buffer so they're processed below
    {
        const auto scope = keyEventFifo.read(keyEventFifo.getNumReady());
        auto drainBlock = [&](int start, int size) {
            for (int i = 0; i < size; ++i)
            {
                const auto& e = keyEventBuf[start + i];
                if (e.note >= 0 && e.note <= 127)
                {
                    if (e.isDown)
                        midiBuffer.addEvent(
                            juce::MidiMessage::noteOn(1, e.note, (juce::uint8)100), 0);
                    else
                        midiBuffer.addEvent(
                            juce::MidiMessage::noteOff(1, e.note, 0.0f), 0);
                }
            }
        };
        drainBlock(scope.startIndex1, scope.blockSize1);
        drainBlock(scope.startIndex2, scope.blockSize2);
    }

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
                synthEngine.releaseAllNotes();
                bassEngine.releaseAllNotes();
                arpCurrentNote = -1;
                arpSequence.clear();
                strumQueue.clear();
                strumSampleDebt = 0;
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
            synthEngine.reset();   // all sound off — immediate for panic
            bassEngine.reset();
            arpCurrentNote = -1;
            arpSequence.clear();
            strumQueue.clear();
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

            // Force bass note into bass register (C1–B2, MIDI 24–47)
            // so it always sounds distinctly lower than the chord notes.
            if (newBass >= 0)
            {
                while (newBass >= 48) newBass -= 12;
                while (newBass < 24) newBass += 12;
            }
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

            // MIDI output routing (always applies)
            midiRouter.routeChord(outMidi, currentNotes, newNotes,
                                  newBass, config, 0);

            const int perfMode = pPerfMode ? pPerfMode->get() : 0;
            const bool bassEnabled = pBassEnabled && pBassEnabled->get();

            // Separate chord notes from bass note for independent engine routing
            std::vector<int> chordOnlyNotes = newNotes;
            if (bassEnabled && newBass >= 0)
                chordOnlyNotes.erase(
                    std::remove(chordOnlyNotes.begin(), chordOnlyNotes.end(), newBass),
                    chordOnlyNotes.end());

            // Bass engine: play bass note immediately regardless of perf mode
            if (bassEnabled)
            {
                if (currentBassNote >= 0)
                    bassEngine.releaseNote(currentBassNote);
                if (newBass >= 0)
                    bassEngine.playNote(newBass, config.velocity);
            }

            // Chord synth: depends on performance mode
            if (perfMode == 0) // Normal
            {
                synthEngine.playChord(chordOnlyNotes, config.velocity);
            }
            else if (perfMode == 1) // Strum
            {
                synthEngine.reset(); // immediate cut — release tails would mask the stagger
                strumQueue = chordOnlyNotes;
                if (pStrumUp && !pStrumUp->get())
                    std::reverse(strumQueue.begin(), strumQueue.end());
                strumSampleDebt = 0; // first note fires at start of next render block
            }
            else // Arpeggiator
            {
                // Release all voices (arp notes aren't tracked in activeNotes)
                synthEngine.releaseAllNotes();
                arpCurrentNote = -1;
                arpNotes    = chordOnlyNotes;
                buildArpSequence();
                arpSeqIdx        = 0;
                arpSampleCounter = 0;
            }

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
        synthEngine.releaseAllNotes();
        bassEngine.releaseAllNotes();
        arpCurrentNote = -1;
        arpSequence.clear();
        strumQueue.clear();
        currentNotes.clear();
        currentBassNote = -1;
        lastChordName   = "";
        displayNotesMask.store(0,  std::memory_order_release);
        displayRootPC.store   (-1, std::memory_order_release);
        displayChordType.store(0,  std::memory_order_release);
    }

    // Build per-block MIDI for strum / arp (fed into synth renderNextBlock)
    juce::MidiBuffer synthMidi;
    const int perfMode   = pPerfMode ? pPerfMode->get() : 0;
    const int blockSize  = buffer.getNumSamples();
    const auto velByte   = (juce::uint8)juce::jlimit(1, 127, (int)(0.8f * 127.0f));

    if (perfMode == 1 && !strumQueue.empty()) // Strum
    {
        // Fire notes at evenly-spaced sample offsets across (potentially multiple) blocks.
        // strumSampleDebt = samples still to wait before the next note fires.
        const int interval = strumIntervalSamples();
        int offset = strumSampleDebt; // position within this block for the next note

        while (!strumQueue.empty())
        {
            if (offset >= blockSize)
            {
                // This note falls in a future block — save the remaining wait time
                strumSampleDebt = offset - blockSize;
                break;
            }
            synthMidi.addEvent(
                juce::MidiMessage::noteOn(1, strumQueue.front(), velByte), offset);
            strumQueue.erase(strumQueue.begin());
            offset += interval;
        }

        if (strumQueue.empty())
            strumSampleDebt = 0;
    }
    else if (perfMode == 2 && !arpSequence.empty()) // Arpeggiator
    {
        const int stepSamples = computeArpStepSamples();
        int pos = 0;
        while (pos < blockSize)
        {
            const int until = stepSamples - arpSampleCounter;
            if (pos + until <= blockSize)
            {
                pos += until;
                arpSampleCounter = 0;
                const int eventPos = juce::jmin(pos, blockSize - 1);

                // Release previous arp note
                if (arpCurrentNote >= 0)
                    synthMidi.addEvent(
                        juce::MidiMessage::noteOff(1, arpCurrentNote, 0.0f), eventPos);

                // Play next note in sequence
                arpCurrentNote = arpSequence[arpSeqIdx % (int)arpSequence.size()];
                ++arpSeqIdx;
                synthMidi.addEvent(
                    juce::MidiMessage::noteOn(1, arpCurrentNote, velByte), eventPos);
            }
            else
            {
                arpSampleCounter += (blockSize - pos);
                pos = blockSize;
            }
        }
    }

    // Render synth audio (only if synth is enabled)
    if (!pSynthEnabled || pSynthEnabled->get())
    {
        juce::MidiBuffer emptyMidi;
        synthEngine.renderNextBlock(buffer, perfMode == 0 ? emptyMidi : synthMidi);

        if (pBassEnabled && pBassEnabled->get())
            bassEngine.renderNextBlock(buffer, emptyMidi);
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
    pBassEngineType  = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::BassEngineType));
    pPerfMode        = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::PerfMode));
    pStrumSpeed      = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::StrumSpeed));
    pStrumUp         = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter(ParamIDs::StrumUp));
    pArpPattern      = dynamic_cast<juce::AudioParameterInt*>  (apvts.getParameter(ParamIDs::ArpPattern));
    pArpBPM          = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::ArpBPM));
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

    if (pBassEngineType)
    {
        int bassIdx = pBassEngineType->get();
        if (bassIdx != prevBassEngine)
        {
            prevBassEngine = bassIdx;
            bassEngine.setEngine(static_cast<BegoniaVoice::Engine>(bassIdx));
        }
    }

    float a = pAttack->get(), d = pDecay->get(), s = pSustain->get(), r = pRelease->get();
    if (a != prevAttack || d != prevDecay || s != prevSustain || r != prevRelease)
    {
        prevAttack = a; prevDecay = d; prevSustain = s; prevRelease = r;
        synthEngine.setADSR(a, d, s, r);
        bassEngine.setADSR(a, d, s, r);
    }

    float cutoff = pFilterCutoff->get(), res = pFilterResonance->get();
    if (cutoff != prevCutoff || res != prevResonance)
    {
        prevCutoff = cutoff; prevResonance = res;
        synthEngine.setFilter(cutoff, res);
        bassEngine.setFilter(cutoff, res);
    }

    float gain = pOutputGain->get();
    if (gain != prevGain)
    {
        prevGain = gain;
        synthEngine.setGain(gain);
        bassEngine.setGain(gain);
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
