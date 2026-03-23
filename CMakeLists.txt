cmake_minimum_required(VERSION 3.22)
project(Orchid VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPM for dependency management
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    OPTIONS
        "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON"
)

CPMAddPackage(
    NAME Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

juce_add_plugin(Orchid
    COMPANY_NAME "OrchidPlugin"
    PLUGIN_MANUFACTURER_CODE Orcd
    PLUGIN_CODE Orch
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Orchid"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES "Instrument|Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(Orchid PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ChordEngine.cpp
    src/VoicingEngine.cpp
    src/SynthEngine.cpp
    src/SynthVoice.cpp
    src/KeyboardMapper.cpp
    src/KeyboardMapperMac.mm
    src/MidiRouter.cpp
    src/ui/ChordButtonPanel.cpp
    src/ui/VoicingPanel.cpp
    src/ui/SynthPanel.cpp
    src/ui/MidiConfigPanel.cpp
    src/ui/KeyboardDisplay.cpp
)

target_compile_definitions(Orchid
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(Orchid
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# macOS framework for global key monitoring
if(APPLE)
    target_link_libraries(Orchid PRIVATE "-framework AppKit")
endif()

# Tests
add_subdirectory(tests)
