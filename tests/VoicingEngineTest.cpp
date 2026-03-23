#include <catch2/catch_test_macros.hpp>
#include "VoicingEngine.h"

static VoicingEngine::VoicingParams makeParams(VoicingEngine::Voicing v, int oct = 0)
{
    VoicingEngine::VoicingParams p;
    p.voicing      = v;
    p.octaveOffset = oct;
    return p;
}

static const std::vector<int> kCmajIntervals = { 0, 4, 7 };         // C major
static const std::vector<int> kCmaj7Intervals = { 0, 4, 7, 11 };    // C major 7

TEST_CASE("VoicingEngine - Close voicing", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Close);

    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 60, 64, 67 });
}

TEST_CASE("VoicingEngine - Inversion 1 (3rd in bass)", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Inversion1);

    // C major: Close = {60, 64, 67}, Inv1 = {64, 67, 72}
    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 64, 67, 72 });
}

TEST_CASE("VoicingEngine - Inversion 2 (5th in bass)", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Inversion2);

    // C major: Inv2 = {67, 72, 76}
    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 67, 72, 76 });
}

TEST_CASE("VoicingEngine - Inversion 3 (7th in bass, 4-note chord)", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Inversion3);

    // Cmaj7: Close = {60,64,67,71}, Inv3 = {71,72,76,79}
    auto notes = engine.applyVoicing(kCmaj7Intervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 71, 72, 76, 79 });
}

TEST_CASE("VoicingEngine - Open voicing", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Open);

    // Close = {60,64,67}, Open = raise index 1 (+12): {60, 67, 76}
    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 60, 67, 76 });
}

TEST_CASE("VoicingEngine - Drop2 voicing", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Drop2);

    // Close = {60,64,67}; 2nd highest = index 1 = 64; drop by 12 → 52
    // Result sorted: {52, 60, 67}
    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 52, 60, 67 });
}

TEST_CASE("VoicingEngine - Octave offset", "[voicing]")
{
    VoicingEngine engine;
    auto params = makeParams(VoicingEngine::Voicing::Close, 1);

    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    REQUIRE(notes == std::vector<int>{ 72, 76, 79 });
}

TEST_CASE("VoicingEngine - MIDI range clamping", "[voicing]")
{
    VoicingEngine engine;
    VoicingEngine::VoicingParams params;
    params.voicing      = VoicingEngine::Voicing::Close;
    params.octaveOffset = 3;    // Push notes very high
    params.midiRangeMin = 36;
    params.midiRangeMax = 96;

    // C4 (60) + 3 octaves = C7 (96) — at boundary, should clamp
    auto notes = engine.applyVoicing(kCmajIntervals, 60, params);
    for (int n : notes)
        REQUIRE(n <= 96);
}

TEST_CASE("VoicingEngine - getBassNote", "[voicing]")
{
    VoicingEngine engine;
    std::vector<int> notes = { 64, 67, 72 };
    REQUIRE(engine.getBassNote(notes) == 64);
}

TEST_CASE("VoicingEngine - empty intervals returns empty", "[voicing]")
{
    VoicingEngine engine;
    VoicingEngine::VoicingParams params;
    auto notes = engine.applyVoicing({}, 60, params);
    REQUIRE(notes.empty());
}
