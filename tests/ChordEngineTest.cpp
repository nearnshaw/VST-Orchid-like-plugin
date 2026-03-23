#include <catch2/catch_test_macros.hpp>
#include "ChordEngine.h"

TEST_CASE("ChordEngine - base triads", "[chord]")
{
    ChordEngine engine;

    SECTION("Major triad")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major, ChordEngine::ExtNone);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7 });
    }

    SECTION("Minor triad")
    {
        auto iv = engine.buildIntervals(ChordEngine::Minor, ChordEngine::ExtNone);
        REQUIRE(iv == std::vector<int>{ 0, 3, 7 });
    }

    SECTION("Sus4 triad")
    {
        auto iv = engine.buildIntervals(ChordEngine::Sus4, ChordEngine::ExtNone);
        REQUIRE(iv == std::vector<int>{ 0, 5, 7 });
    }

    SECTION("Diminished triad")
    {
        auto iv = engine.buildIntervals(ChordEngine::Dim, ChordEngine::ExtNone);
        REQUIRE(iv == std::vector<int>{ 0, 3, 6 });
    }
}

TEST_CASE("ChordEngine - extensions on Major", "[chord]")
{
    ChordEngine engine;

    SECTION("Major + Add6 = {0,4,7,9}")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major, ChordEngine::Add6);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7, 9 });
    }

    SECTION("Major + m7 = {0,4,7,10} (dom7)")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major, ChordEngine::AddM7);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7, 10 });
    }

    SECTION("Major + Maj7 = {0,4,7,11}")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major, ChordEngine::AddMaj7);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7, 11 });
    }

    SECTION("Major + Add9 = {0,4,7,14}")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major, ChordEngine::Add9);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7, 14 });
    }

    SECTION("Major + Maj7 + Add9 = {0,4,7,11,14} (maj9)")
    {
        auto iv = engine.buildIntervals(ChordEngine::Major,
                                        ChordEngine::AddMaj7 | ChordEngine::Add9);
        REQUIRE(iv == std::vector<int>{ 0, 4, 7, 11, 14 });
    }
}

TEST_CASE("ChordEngine - extensions on Dim", "[chord]")
{
    ChordEngine engine;

    SECTION("Dim + m7 = fully diminished 7 {0,3,6,9}")
    {
        auto iv = engine.buildIntervals(ChordEngine::Dim, ChordEngine::AddM7);
        REQUIRE(iv == std::vector<int>{ 0, 3, 6, 9 });
    }

    SECTION("Dim + Add6 = also {0,3,6,9} (same interval)")
    {
        auto iv = engine.buildIntervals(ChordEngine::Dim, ChordEngine::Add6);
        REQUIRE(iv == std::vector<int>{ 0, 3, 6, 9 });
    }
}

TEST_CASE("ChordEngine - no chord type returns root only", "[chord]")
{
    ChordEngine engine;
    auto iv = engine.buildIntervals(ChordEngine::None, ChordEngine::ExtNone);
    REQUIRE(iv == std::vector<int>{ 0 });
}

TEST_CASE("ChordEngine - buildNotes offsets by root", "[chord]")
{
    ChordEngine engine;

    SECTION("C4 (MIDI 60) Major = {60, 64, 67}")
    {
        auto notes = engine.buildNotes(60, ChordEngine::Major, ChordEngine::ExtNone);
        REQUIRE(notes == std::vector<int>{ 60, 64, 67 });
    }

    SECTION("D4 (MIDI 62) Minor = {62, 65, 69}")
    {
        auto notes = engine.buildNotes(62, ChordEngine::Minor, ChordEngine::ExtNone);
        REQUIRE(notes == std::vector<int>{ 62, 65, 69 });
    }
}

TEST_CASE("ChordEngine - chord naming", "[chord]")
{
    REQUIRE(ChordEngine::getChordName(60, ChordEngine::Major, ChordEngine::ExtNone)   == "Cmaj");
    REQUIRE(ChordEngine::getChordName(60, ChordEngine::Minor, ChordEngine::ExtNone)   == "Cm");
    REQUIRE(ChordEngine::getChordName(60, ChordEngine::Major, ChordEngine::AddMaj7)   == "Cmaj7");
    REQUIRE(ChordEngine::getChordName(62, ChordEngine::Minor, ChordEngine::AddM7)     == "Dm7");
    REQUIRE(ChordEngine::getChordName(60, ChordEngine::Dim,   ChordEngine::AddM7)     == "Cdim7");
}
