#if defined(__APPLE__)

#include "KeyboardMapper.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

// macOS key codes for our mapped keys
// These are hardware key codes (layout-independent)
static const std::unordered_map<unsigned short, int> kMacKeyCodeToJuce = {
    { 18, '1' },   // Key "1"
    { 19, '2' },   // Key "2"
    { 20, '3' },   // Key "3"
    { 21, '4' },   // Key "4"
    { 12, 'q' },   // Key "Q"
    { 13, 'w' },   // Key "W"
    { 14, 'e' },   // Key "E"
    { 15, 'r' },   // Key "R"
};

void KeyboardMapper::installGlobalMonitor()
{
    if (globalMonitorRef != nullptr)
        return;

    // Capture 'self' via __block pointer to avoid retain cycle
    KeyboardMapper* mapper = this;

    id monitor = [NSEvent
        addGlobalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskKeyUp)
        handler:^(NSEvent* event) {
            unsigned short macCode = event.keyCode;
            auto it = kMacKeyCodeToJuce.find(macCode);
            if (it == kMacKeyCodeToJuce.end())
                return;

            int juceCode = it->second;
            bool isDown = (event.type == NSEventTypeKeyDown);

            // Dispatch to message thread to stay thread-safe
            juce::MessageManager::callAsync([mapper, juceCode, isDown]() {
                if (isDown)
                    mapper->handleKeyDown(juce::KeyPress(juceCode));
                else
                    mapper->handleKeyUp(juceCode);
            });
        }];

    globalMonitorRef = (void*)CFBridgingRetain(monitor);
    globalMonitorActive.store(true, std::memory_order_release);
}

void KeyboardMapper::removeGlobalMonitor()
{
    if (globalMonitorRef == nullptr)
        return;

    id monitor = (id)CFBridgingRelease(globalMonitorRef);
    [NSEvent removeMonitor:monitor];
    globalMonitorRef = nullptr;
    globalMonitorActive.store(false, std::memory_order_release);
}

int KeyboardMapper::macKeyCodeToJuce(int macKeyCode)
{
    auto it = kMacKeyCodeToJuce.find((unsigned short)macKeyCode);
    return (it != kMacKeyCodeToJuce.end()) ? it->second : -1;
}

void KeyboardMapper::pollKeyStates()
{
    // Ordered to match kMacKeyCodeToJuce: 1, 2, 3, 4, q, w, e, r
    struct KeyDef { CGKeyCode cgCode; int juceCode; };
    static constexpr KeyDef kKeys[] = {
        { 18, '1' }, { 19, '2' }, { 20, '3' }, { 21, '4' },
        { 12, 'q' }, { 13, 'w' }, { 14, 'e' }, { 15, 'r' }
    };
    static constexpr int kCount = 8;

    // Build a bitmask of currently-held keys via the HID layer.
    // CGEventSourceKeyState bypasses JUCE's keysCurrentlyDown (which is wiped
    // whenever any modifier key changes, causing false key-up events).
    uint8_t newMask = 0;
    for (int i = 0; i < kCount; ++i)
        if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kKeys[i].cgCode))
            newMask |= uint8_t(1 << i);

    static uint8_t prevMask = 0;
    const uint8_t pressed  = newMask & ~prevMask;   // newly down since last poll
    const uint8_t released = prevMask & ~newMask;   // newly up since last poll
    prevMask = newMask;

    for (int i = 0; i < kCount; ++i)
    {
        if (pressed  & (1 << i)) handleKeyDown(juce::KeyPress(kKeys[i].juceCode));
        if (released & (1 << i)) handleKeyUp(kKeys[i].juceCode);
    }
}

#endif // __APPLE__
