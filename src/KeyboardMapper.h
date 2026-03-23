#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "ChordEngine.h"
#include <atomic>
#include <deque>
#include <unordered_map>

class KeyboardMapper
{
public:
    struct ChordState
    {
        uint8_t chordType  = 0;
        uint8_t extensions = 0;
    };

    // Packed atomic: high byte = chordType, low byte = extensions
    struct AtomicChordState
    {
        std::atomic<uint16_t> packed { 0 };

        void set(uint8_t type, uint8_t ext)
        {
            packed.store((uint16_t(type) << 8) | uint16_t(ext),
                         std::memory_order_release);
        }

        ChordState get() const
        {
            uint16_t val = packed.load(std::memory_order_acquire);
            return { uint8_t(val >> 8), uint8_t(val & 0xFF) };
        }
    };

    KeyboardMapper();
    ~KeyboardMapper();

    // Call from PluginEditor KeyListener callbacks
    bool handleKeyDown (const juce::KeyPress& key);
    bool handleKeyUp   (int keyCode);   // juce::KeyPress::keyCode

    // Thread-safe read (safe to call from audio thread)
    ChordState getCurrentState() const { return atomicState.get(); }

    // Enable/disable macOS global NSEvent monitor
    void setGlobalMonitorEnabled (bool enable);
    bool isGlobalMonitorActive   () const { return globalMonitorActive.load(); }

    // Direct injection (called when user clicks a chord button in UI)
    void injectChordType (uint8_t type, bool isDown);
    void injectExtension (uint8_t ext,  bool isDown);

private:
    // Key → (isExtension, bit)
    struct KeyAction { bool isExtension; uint8_t bit; };
    static const std::unordered_map<int, KeyAction> kKeyMap;

    // UI-thread state (mutable only from UI thread)
    uint8_t heldChordTypes = 0;
    uint8_t heldExtensions = 0;
    std::deque<int> chordTypePressOrder;   // for "last pressed wins"

    AtomicChordState atomicState;
    std::atomic<bool> globalMonitorActive { false };

    void recomputeState();

    // macOS global monitor (implemented in KeyboardMapperMac.mm)
    void* globalMonitorRef = nullptr;
    void  installGlobalMonitor();
    void  removeGlobalMonitor();

    // Maps raw macOS key codes to JUCE key codes (set by .mm file)
    static int macKeyCodeToJuce(int macKeyCode);
};
