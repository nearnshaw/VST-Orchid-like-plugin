#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "OrchidParameters.h"
#include "ChordEngine.h"
#include "VoicingEngine.h"
#include "SynthEngine.h"
#include "MidiRouter.h"
#include "KeyboardMapper.h"

class BegoniaProcessor : public juce::AudioProcessor
{
public:
    BegoniaProcessor();
    ~BegoniaProcessor() override;

    // AudioProcessor interface
    void prepareToPlay     (double sampleRate, int samplesPerBlock) override;
    void releaseResources  () override;
    void processBlock      (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Begonia"; }

    bool   acceptsMidi()  const override { return true;  }
    bool   producesMidi() const override { return true;  }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int  getNumPrograms()                              override { return 1; }
    int  getCurrentProgram()                           override { return 0; }
    void setCurrentProgram(int)                        override {}
    const juce::String getProgramName(int)             override { return "Default"; }
    void changeProgramName(int, const juce::String&)   override {}

    void getStateInformation (juce::MemoryBlock& data) override;
    void setStateInformation (const void* data, int size) override;

    // Accessors for editor
    juce::AudioProcessorValueTreeState& getAPVTS()         { return apvts; }
    KeyboardMapper&                     getKeyboardMapper() { return keyboardMapper; }
    ChordEngine&                        getChordEngine()    { return chordEngine; }

    // For UI display: chord name updated on audio thread, read by UI
    juce::String getLastChordName() const { return lastChordName; }
    int          getLastRootNote()  const { return lastRootNote.load(); }

    struct ChordDisplayState {
        int      rootPitchClass;   // -1 = no chord
        uint8_t  chordType;
        uint16_t notesBitmask;     // bit i set = pitch class i is in the chord
    };
    ChordDisplayState getChordDisplayState() const {
        return { (int)displayRootPC.load(), displayChordType.load(), displayNotesMask.load() };
    }

    // Bus layout
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

private:
    juce::AudioProcessorValueTreeState apvts;

    // Core engines
    ChordEngine    chordEngine;
    VoicingEngine  voicingEngine;
    SynthEngine    synthEngine;
    MidiRouter     midiRouter;
    KeyboardMapper keyboardMapper;

    // Audio-thread state
    int              currentRootNote   = -1;
    std::vector<int> currentNotes;
    int              currentBassNote   = -1;

    // Lock-free chord state: keyboard mapper (UI thread) → audio thread
    // Packed as uint16_t: high byte = chordType, low byte = extensions
    // Use a simple atomic since the value fits in 16 bits
    // (ChordState is set from UI thread via keyboardMapper, read here)

    // Cached parameter raw pointers (set in prepareToPlay, read in processBlock)
    juce::AudioParameterInt*   pVoicingType     = nullptr;
    juce::AudioParameterInt*   pOctaveOffset    = nullptr;
    juce::AudioParameterBool*  pBassEnabled     = nullptr;
    juce::AudioParameterInt*   pSynthEngine     = nullptr;
    juce::AudioParameterFloat* pAttack          = nullptr;
    juce::AudioParameterFloat* pDecay           = nullptr;
    juce::AudioParameterFloat* pSustain         = nullptr;
    juce::AudioParameterFloat* pRelease         = nullptr;
    juce::AudioParameterFloat* pFilterCutoff    = nullptr;
    juce::AudioParameterFloat* pFilterResonance = nullptr;
    juce::AudioParameterInt*   pChordChannel    = nullptr;
    juce::AudioParameterInt*   pBassChannel     = nullptr;
    juce::AudioParameterBool*  pMidiOutEnabled  = nullptr;
    juce::AudioParameterFloat* pOutputGain      = nullptr;
    juce::AudioParameterBool*  pGlobalKeyMon    = nullptr;

    // Previous parameter values to detect changes (avoid unnecessary updates)
    int   prevSynthEngine  = -1;
    float prevAttack       = -1.0f;
    float prevDecay        = -1.0f;
    float prevSustain      = -1.0f;
    float prevRelease      = -1.0f;
    float prevCutoff       = -1.0f;
    float prevResonance    = -1.0f;
    float prevGain         = -1.0f;
    bool  prevGlobalKeyMon = false;

    // For UI display (read from timer callback on message thread)
    juce::String          lastChordName;
    std::atomic<int>      lastRootNote     { -1 };
    // Circle of fifths display state (written on audio thread, read on UI thread)
    std::atomic<int8_t>   displayRootPC    { -1 };   // pitch class, -1 = none
    std::atomic<uint8_t>  displayChordType { 0 };
    std::atomic<uint16_t> displayNotesMask { 0 };    // bit i = pitch class i is sounding

    void cacheParameterPointers();
    void updateSynthParameters();
    MidiRouter::Config buildRouterConfig() const;
    VoicingEngine::VoicingParams buildVoicingParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BegoniaProcessor)
};
