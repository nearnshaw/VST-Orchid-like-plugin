#if defined(__APPLE__)

#include "KeyboardMapper.h"
#import <AppKit/AppKit.h>

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

#endif // __APPLE__
