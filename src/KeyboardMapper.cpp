#include "KeyboardMapper.h"
#include <algorithm>

// Key map: JUCE key codes → chord type / extension bits
// Key codes: '1'=49, '2'=50, '3'=51, '4'=52
//            'q'=113, 'w'=119, 'e'=101, 'r'=114 (lowercase JUCE codes)
const std::unordered_map<int, KeyboardMapper::KeyAction> KeyboardMapper::kKeyMap = {
    { '1', { false, ChordEngine::Major } },
    { '2', { false, ChordEngine::Minor } },
    { '3', { false, ChordEngine::Sus4  } },
    { '4', { false, ChordEngine::Dim   } },
    { 'q', { true,  ChordEngine::Add6    } },
    { 'w', { true,  ChordEngine::AddM7   } },
    { 'e', { true,  ChordEngine::AddMaj7 } },
    { 'r', { true,  ChordEngine::Add9    } },
    // Also handle uppercase (shift held)
    { 'Q', { true,  ChordEngine::Add6    } },
    { 'W', { true,  ChordEngine::AddM7   } },
    { 'E', { true,  ChordEngine::AddMaj7 } },
    { 'R', { true,  ChordEngine::Add9    } },
};

KeyboardMapper::KeyboardMapper() = default;

KeyboardMapper::~KeyboardMapper()
{
    removeGlobalMonitor();
}

bool KeyboardMapper::handleKeyDown(const juce::KeyPress& key)
{
    // Try both the key code and the character
    int code = key.getKeyCode();
    // Normalise to lowercase for letters
    if (code >= 'A' && code <= 'Z')
        code = code - 'A' + 'a';

    auto it = kKeyMap.find(code);
    if (it == kKeyMap.end())
    {
        // Also try original code
        it = kKeyMap.find(key.getKeyCode());
        if (it == kKeyMap.end())
            return false;
    }

    const auto& action = it->second;

    if (!action.isExtension)
    {
        heldChordTypes |= action.bit;
        // Track press order for "last pressed wins"
        chordTypePressOrder.erase(
            std::remove(chordTypePressOrder.begin(), chordTypePressOrder.end(),
                        static_cast<int>(action.bit)),
            chordTypePressOrder.end());
        chordTypePressOrder.push_back(static_cast<int>(action.bit));
    }
    else
    {
        // W (AddM7) and E (AddMaj7) are mutually exclusive — a chord can't have
        // both a dominant 7th and a major 7th simultaneously.
        if (action.bit == ChordEngine::AddM7)
            heldExtensions &= ~ChordEngine::AddMaj7;
        else if (action.bit == ChordEngine::AddMaj7)
            heldExtensions &= ~ChordEngine::AddM7;
        heldExtensions |= action.bit;
    }

    recomputeState();
    return true;
}

bool KeyboardMapper::handleKeyUp(int keyCode)
{
    int code = keyCode;
    if (code >= 'A' && code <= 'Z')
        code = code - 'A' + 'a';

    auto it = kKeyMap.find(code);
    if (it == kKeyMap.end())
    {
        it = kKeyMap.find(keyCode);
        if (it == kKeyMap.end())
            return false;
    }

    const auto& action = it->second;

    if (!action.isExtension)
    {
        heldChordTypes &= ~action.bit;
        chordTypePressOrder.erase(
            std::remove(chordTypePressOrder.begin(), chordTypePressOrder.end(),
                        static_cast<int>(action.bit)),
            chordTypePressOrder.end());
    }
    else
    {
        heldExtensions &= ~action.bit;
    }

    recomputeState();
    return true;
}

void KeyboardMapper::injectChordType(uint8_t type, bool isDown)
{
    if (isDown)
    {
        heldChordTypes |= type;
        chordTypePressOrder.erase(
            std::remove(chordTypePressOrder.begin(), chordTypePressOrder.end(),
                        static_cast<int>(type)),
            chordTypePressOrder.end());
        chordTypePressOrder.push_back(static_cast<int>(type));
    }
    else
    {
        heldChordTypes &= ~type;
        chordTypePressOrder.erase(
            std::remove(chordTypePressOrder.begin(), chordTypePressOrder.end(),
                        static_cast<int>(type)),
            chordTypePressOrder.end());
    }
    recomputeState();
}

void KeyboardMapper::injectExtension(uint8_t ext, bool isDown)
{
    if (isDown)
    {
        if (ext == ChordEngine::AddM7)
            heldExtensions &= ~ChordEngine::AddMaj7;
        else if (ext == ChordEngine::AddMaj7)
            heldExtensions &= ~ChordEngine::AddM7;
        heldExtensions |= ext;
    }
    else
        heldExtensions &= ~ext;
    recomputeState();
}

void KeyboardMapper::recomputeState()
{
    // "Last pressed wins" for chord type: use the most recently pressed held key
    uint8_t resolvedType = 0;
    if (!chordTypePressOrder.empty())
        resolvedType = static_cast<uint8_t>(chordTypePressOrder.back());

    atomicState.set(resolvedType, heldExtensions);
}

void KeyboardMapper::setGlobalMonitorEnabled(bool enable)
{
    if (enable && !globalMonitorActive.load())
        installGlobalMonitor();
    else if (!enable && globalMonitorActive.load())
        removeGlobalMonitor();
}

// Stub implementations — actual code in KeyboardMapperMac.mm (macOS only)
#if !defined(__APPLE__)
void KeyboardMapper::installGlobalMonitor() {}
void KeyboardMapper::removeGlobalMonitor()  {}
int  KeyboardMapper::macKeyCodeToJuce(int)  { return -1; }
#endif
