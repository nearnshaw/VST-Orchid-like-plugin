add_executable(OrchidTests
    ChordEngineTest.cpp
    VoicingEngineTest.cpp
)

target_link_libraries(OrchidTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_basics
        juce::juce_audio_processors
)

target_include_directories(OrchidTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link the non-plugin source files directly (no JUCE plugin boilerplate needed)
target_sources(OrchidTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ChordEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/VoicingEngine.cpp
)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(OrchidTests)
