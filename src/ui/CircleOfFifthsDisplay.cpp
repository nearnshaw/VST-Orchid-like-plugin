#include "CircleOfFifthsDisplay.h"
#include "../OrchidParameters.h"
#include <cmath>

// -----------------------------------------------------------------------
// Static data
// -----------------------------------------------------------------------

// Circle positions, clockwise from C at the top.
// Each entry: { majorRoot, minorRoot, dimRoot, majorName, minorName, dimName }
// All names use # notation only (no ♭).
const std::array<CircleOfFifthsDisplay::CoFPos, 12> CircleOfFifthsDisplay::kPos = {{
    {  0,  9, 11, "C",  "Am",  "Bdim"  },   // 0  – key of C
    {  7,  4,  6, "G",  "Em",  "F#dim" },   // 1  – key of G
    {  2, 11,  1, "D",  "Bm",  "C#dim" },   // 2  – key of D
    {  9,  6,  8, "A",  "F#m", "G#dim" },   // 3  – key of A
    {  4,  1,  3, "E",  "C#m", "D#dim" },   // 4  – key of E
    { 11,  8, 10, "B",  "G#m", "A#dim" },   // 5  – key of B
    {  6,  3,  5, "F#", "D#m", "Fdim"  },   // 6  – key of F#
    {  1, 10,  0, "C#", "A#m", "Cdim"  },   // 7  – key of C#
    {  8,  5,  7, "G#", "Fm",  "Gdim"  },   // 8  – key of G#
    {  3,  0,  2, "D#", "Cm",  "Ddim"  },   // 9  – key of D#
    { 10,  7,  9, "A#", "Gm",  "Adim"  },   // 10 – key of A#
    {  5,  2,  4, "F",  "Dm",  "Edim"  },   // 11 – key of F
}};

// Pitch class (0=C … 11=B) → circle position
const std::array<int, 12> CircleOfFifthsDisplay::kPCtoPos = {
//   C   C#  D   D#  E   F   F#  G   G#  A   A#  B
     0,  7,  2,  9,  4, 11,  6,  1,  8,  3, 10,  5
};

// -----------------------------------------------------------------------
// Colours
// -----------------------------------------------------------------------
const juce::Colour CircleOfFifthsDisplay::kBg            = juce::Colour(0xFF12121E);
const juce::Colour CircleOfFifthsDisplay::kBase[3]       = {
    juce::Colour(0xFF1E1E30),   // major – dark neutral
    juce::Colour(0xFF181828),   // minor – slightly darker
    juce::Colour(0xFF141422),   // dim   – darkest
};
const juce::Colour CircleOfFifthsDisplay::kDiatonic[3]   = {
    juce::Colour(0xFF183828),   // major – teal
    juce::Colour(0xFF1A3A62),   // minor – brighter steel blue
    juce::Colour(0xFF3C1A54),   // dim   – brighter purple
};
const juce::Colour CircleOfFifthsDisplay::kTonic         = juce::Colour(0xFF2C2010);
const juce::Colour CircleOfFifthsDisplay::kScaleTone     = juce::Colour(0xFF1C2230);  // faint blue — in scale but no triad
const juce::Colour CircleOfFifthsDisplay::kActive[3]     = {
    juce::Colour(0xFFE06B3A),   // major – orange accent
    juce::Colour(0xFF4A7AC8),   // minor – blue
    juce::Colour(0xFF9045B5),   // dim   – purple
};
const juce::Colour CircleOfFifthsDisplay::kActiveNonDiat = juce::Colour(0xFF992222);  // red — chord is outside the scale
const juce::Colour CircleOfFifthsDisplay::kNoteHint      = juce::Colour(0xFF3A2C18);  // diatonic chord tone (on its ring)
const juce::Colour CircleOfFifthsDisplay::kNoteOutOfScale= juce::Colour(0xFF4A1515);  // chord tone not in the scale
const juce::Colour CircleOfFifthsDisplay::kSep           = juce::Colour(0xFF26263E);
const juce::Colour CircleOfFifthsDisplay::kText[3]       = {
    juce::Colour(0xFFD8D8EE),   // major text – clearly readable on dark bg
    juce::Colour(0xFFC8C8E4),   // minor text
    juce::Colour(0xFFB8B8D8),   // dim text
};
const juce::Colour CircleOfFifthsDisplay::kTextActive    = juce::Colours::white;
const juce::Colour CircleOfFifthsDisplay::kTextDiatonic  = juce::Colour(0xFF88DDAA);
const juce::Colour CircleOfFifthsDisplay::kTextTonic     = juce::Colour(0xFFE0A855);
const juce::Colour CircleOfFifthsDisplay::kTextScaleTone  = juce::Colour(0xFF7080A0);  // dim bluish
const juce::Colour CircleOfFifthsDisplay::kTextOutOfScale = juce::Colour(0xFFDD7777);  // pinkish-red

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------
CircleOfFifthsDisplay::CircleOfFifthsDisplay(juce::AudioProcessorValueTreeState& apvts_)
    : apvts(apvts_)
{
    // Root note dropdown (all sharps, C=0 … B=11)
    static const char* keyNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(keyNames[i], i + 1);   // item IDs are 1-indexed
    keyCombo.setSelectedId(1, juce::dontSendNotification);
    keyAtt = std::make_unique<ComboAtt>(apvts_, ParamIDs::SelectedKey, keyCombo);
    {
        const int id = keyCombo.getSelectedId();
        m_key = juce::jlimit(0, 11, id > 0 ? id - 1 : 0);
    }
    keyCombo.onChange = [this]()
    {
        const int id = keyCombo.getSelectedId();
        m_key = juce::jlimit(0, 11, id > 0 ? id - 1 : 0);
        repaint();
    };

    // Scale/mode dropdown (item ID = index + 1)
    static const char* scaleNames[] = {
        "Major", "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian",
        "Locrian", "Harm. Minor", "Mel. Minor", "Lydian Dom", "Phryg. Dom", "Hungarian",
        "Mxlyd b6"
    };
    for (int i = 0; i < 13; ++i)
        scaleCombo.addItem(scaleNames[i], i + 1);
    scaleCombo.setSelectedId(1, juce::dontSendNotification);
    scaleAtt = std::make_unique<ComboAtt>(apvts_, ParamIDs::SelectedScale, scaleCombo);
    {
        const int id = scaleCombo.getSelectedId();
        m_scale = juce::jlimit(0, 12, id > 0 ? id - 1 : 0);
    }
    scaleCombo.onChange = [this]()
    {
        const int id = scaleCombo.getSelectedId();
        m_scale = juce::jlimit(0, 12, id > 0 ? id - 1 : 0);
        repaint();
    };

    auto styleLabel = [this](juce::Label& lbl)
    {
        lbl.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        lbl.setColour(juce::Label::textColourId, kText[0].withAlpha(0.7f));
        lbl.setJustificationType(juce::Justification::centredRight);
    };
    styleLabel(keyLabel);
    styleLabel(scaleLabel);

    addAndMakeVisible(keyLabel);
    addAndMakeVisible(keyCombo);
    addAndMakeVisible(scaleLabel);
    addAndMakeVisible(scaleCombo);
}

// -----------------------------------------------------------------------
// Public state setters
// -----------------------------------------------------------------------
void CircleOfFifthsDisplay::setCurrentChord(int rootPitchClass, uint8_t chordType,
                                              uint16_t notesBitmask)
{
    if (activeRoot != rootPitchClass || activeType != chordType || activeMask != notesBitmask)
    {
        activeRoot = rootPitchClass;
        activeType = chordType;
        activeMask = notesBitmask;
        repaint();
    }
}

void CircleOfFifthsDisplay::clearChord()
{
    if (activeRoot != -1 || activeMask != 0)
    {
        activeRoot = -1;
        activeType = 0;
        activeMask = 0;
        repaint();
    }
}

// -----------------------------------------------------------------------
// resized
// -----------------------------------------------------------------------
void CircleOfFifthsDisplay::resized()
{
    const int w   = getWidth();
    const int h   = getHeight();
    const int pad = 6;

    // Two-combo selector strip at top:  [ROOT ▼]  [SCALE ▼]
    const int selectorH = 26;
    const int keyLblW   = 34;
    const int keyW      = 56;
    const int scaleLblW = 42;
    const int scaleW    = 112;
    const int gap       = 8;
    const int totalW    = keyLblW + keyW + gap + scaleLblW + scaleW;
    int x = (w - totalW) / 2;

    keyLabel .setBounds(x,                             pad, keyLblW,   selectorH);
    keyCombo .setBounds(x + keyLblW,                   pad, keyW,      selectorH);
    scaleLabel.setBounds(x + keyLblW + keyW + gap,     pad, scaleLblW, selectorH);
    scaleCombo.setBounds(x + keyLblW + keyW + gap + scaleLblW, pad, scaleW, selectorH);

    // Circle area below selector
    const int circleY  = pad + selectorH + pad;
    const int circleH  = h - circleY - pad;
    const int circleW  = w - pad * 2;
    const int diameter = std::min(circleW, circleH);

    centre = { (float)(pad + circleW / 2), (float)(circleY + circleH / 2) };
    outerR = diameter * 0.5f;
}

// -----------------------------------------------------------------------
// Drawing helpers
// -----------------------------------------------------------------------
juce::Path CircleOfFifthsDisplay::makeSegment(float a1, float a2, float r1, float r2) const
{
    juce::Path p;
    p.addCentredArc(centre.x, centre.y, r2, r2, 0.0f, a1, a2, true);
    p.addCentredArc(centre.x, centre.y, r1, r1, 0.0f, a2, a1, false);
    p.closeSubPath();
    return p;
}

juce::Point<float> CircleOfFifthsDisplay::arcMidpoint(float a1, float a2, float r) const
{
    float mid = (a1 + a2) * 0.5f;
    return { centre.x + r * std::cos(mid), centre.y + r * std::sin(mid) };
}

void CircleOfFifthsDisplay::drawSegmentLabel(juce::Graphics& g, const juce::String& text,
                                              juce::Point<float> pt, float size, bool bold,
                                              juce::Colour colour, float textBoxWidth) const
{
    g.setColour(colour);
    g.setFont(bold ? juce::FontOptions(size).withStyle("Bold") : juce::FontOptions(size));
    const float th = size + 4.0f;
    g.drawText(text,
               juce::Rectangle<float>(pt.x - textBoxWidth * 0.5f, pt.y - th * 0.5f,
                                      textBoxWidth, th),
               juce::Justification::centred, false);
}

// -----------------------------------------------------------------------
// Music logic
// -----------------------------------------------------------------------
int CircleOfFifthsDisplay::selectedKey() const
{
    return m_key;   // updated in onChange; never calls into APVTS or combo internals
}

// Diatonic chord roots by scale and ring type.
// Each row lists semitone offsets from root K; -1 = sentinel (end of list).
// ring 0=major, ring 1=minor, ring 2=dim.
// Scales: 0=Major, 1=Nat.Minor, 2=Dorian, 3=Phrygian, 4=Lydian, 5=Mixolydian,
//         6=Locrian, 7=Harm.Minor, 8=Mel.Minor, 9=Lydian Dom, 10=Phryg.Dom,
//         11=Hungarian, 12=Mixolydian b6
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

// Which ring hosts the tonic (i/I/i°) chord for each scale
static const int kTonicRing[13] = {
    0, // Major          — I  is major
    1, // Natural Minor  — i  is minor
    1, // Dorian         — i  is minor
    1, // Phrygian       — i  is minor
    0, // Lydian         — I  is major
    0, // Mixolydian     — I  is major
    2, // Locrian        — i° is dim
    1, // Harmonic Minor — i  is minor
    1, // Melodic Minor  — i  is minor
    0, // Lydian Dom     — I  is major
    0, // Phrygian Dom   — I  is major
    1, // Hungarian Min  — i  is minor
    0, // Mixolydian b6  — I  is major
};

// Scale-tone-only pitch-class offsets from K: in the scale but no standard triad.
// These come from scale degrees that only form augmented triads (ignored in our system).
// -1 = sentinel.
static const int kScaleOnly[13][3] = {
    {-1,-1,-1}, // Major          — all scale tones have triads
    {-1,-1,-1}, // Natural Minor
    {-1,-1,-1}, // Dorian
    {-1,-1,-1}, // Phrygian
    {-1,-1,-1}, // Lydian
    {-1,-1,-1}, // Mixolydian
    {-1,-1,-1}, // Locrian
    { 3,-1,-1}, // Harmonic Minor — bIII (augmented)
    { 3,-1,-1}, // Melodic Minor  — bIII (augmented)
    {10,-1,-1}, // Lydian Dom     — bVII (augmented)
    { 8,-1,-1}, // Phrygian Dom   — bVI  (augmented)
    { 2, 3,-1}, // Hungarian Min  — II and bIII (no standard triads)
    { 8,-1,-1}, // Mixolydian b6  — bVI  (augmented)
};

static bool isDiatonicK(int pc, int ring, int K, int scale)
{
    const int (*table)[4] = (ring == 0) ? kMajOffs
                          : (ring == 1) ? kMinOffs
                          :               kDimOffs;
    const int s = juce::jlimit(0, 12, scale);
    for (int j = 0; j < 4; ++j)
    {
        if (table[s][j] < 0) break;
        if (pc == (K + table[s][j]) % 12) return true;
    }
    return false;
}

static bool isScaleToneOnlyK(int pc, int K, int scale)
{
    const int s = juce::jlimit(0, 12, scale);
    for (int j = 0; j < 3; ++j)
    {
        if (kScaleOnly[s][j] < 0) break;
        if (pc == (K + kScaleOnly[s][j]) % 12) return true;
    }
    return false;
}

bool CircleOfFifthsDisplay::isDiatonic(int pc, int ring) const
{
    return isDiatonicK(pc, ring, m_key, m_scale);
}

bool CircleOfFifthsDisplay::isActive(int pos, int ring) const
{
    if (activeRoot < 0) return false;
    const auto& p = kPos[pos];
    if (ring == 0) return (activeType & ChordEngine::Major) && p.majorRoot == activeRoot;
    if (ring == 1) return (activeType & ChordEngine::Minor) && p.minorRoot == activeRoot;
    if (ring == 2) return (activeType & ChordEngine::Dim)   && p.dimRoot   == activeRoot;
    return false;
}

bool CircleOfFifthsDisplay::isChordNote(int pc) const
{
    if (activeMask == 0) return false;
    return (activeMask & (uint16_t(1) << pc)) != 0;
}

// -----------------------------------------------------------------------
// Colour selection
// Priority: active > chord-tone-hint > tonic > diatonic > scale-tone > base
// -----------------------------------------------------------------------
juce::Colour CircleOfFifthsDisplay::segmentColour(int pos, int ring, int key, int scale) const
{
    const int s  = juce::jlimit(0, 12, scale);
    const int pc = (ring == 0) ? kPos[pos].majorRoot
                 : (ring == 1) ? kPos[pos].minorRoot
                 :               kPos[pos].dimRoot;

    // Active chord — colour changes if it's outside the selected scale
    if (isActive(pos, ring))
        return isDiatonicK(pc, ring, key, s) ? kActive[ring] : kActiveNonDiat;

    // Chord tone hints: show on whichever ring matches the pc's diatonic role.
    // If the pc is in none of the diatonic rings, flag it on the outer ring.
    if (activeRoot >= 0 && isChordNote(pc))
    {
        const bool dMaj = isDiatonicK(pc, 0, key, s);
        const bool dMin = isDiatonicK(pc, 1, key, s);
        const bool dDim = isDiatonicK(pc, 2, key, s);

        if (ring == 0 && dMaj) return kNoteHint;
        if (ring == 1 && dMin) return kNoteHint;
        if (ring == 2 && dDim) return kNoteHint;
        // Not diatonic in any ring — show on the outer ring as a warning
        if (ring == 0 && !dMaj && !dMin && !dDim) return kNoteOutOfScale;
    }

    // Tonic accent on the ring that hosts the tonic chord for this scale
    if (ring == kTonicRing[s] && activeRoot < 0 && pc == key)
        return kTonic;

    if (isDiatonicK(pc, ring, key, s))
        return kDiatonic[ring];

    if (isScaleToneOnlyK(pc, key, s))
        return kScaleTone;

    return kBase[ring];
}

// -----------------------------------------------------------------------
// paint
// -----------------------------------------------------------------------
void CircleOfFifthsDisplay::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    if (outerR < 10.0f) return;

    const float pi2      = juce::MathConstants<float>::twoPi;
    const float halfPi   = juce::MathConstants<float>::halfPi;
    const float segAngle = pi2 / 12.0f;
    // startAngle drives label midpoints via arcMidpoint() (standard cos/sin, 0 = right).
    // JUCE's addCentredArc uses the same convention, but empirically the arcs are
    // offset by +π/2 relative to arcMidpoint, so fills use arcStartAngle instead.
    const float startAngle    = -halfPi - segAngle * 0.5f;  // for arcMidpoint / labels
    const float arcStartAngle = startAngle + halfPi;         // for makeSegment / fills
    // Small gap between segments for readability
    const float gap      = 0.012f;

    // Snapshot key and scale once for the whole frame
    const int key   = m_key;
    const int scale = m_scale;

    // Precomputed ring absolute radii
    const float rMajO = outerR * kMajOuter;
    const float rMajI = outerR * kMajInner;
    const float rMinO = outerR * kMinOuter;
    const float rMinI = outerR * kMinInner;
    const float rDimO = outerR * kDimOuter;
    const float rDimI = outerR * kDimInner;

    // Text radii (arc midpoints)
    const float rMajTxt = outerR * (kMajOuter + kMajInner) * 0.5f;
    const float rMinTxt = outerR * (kMinOuter + kMinInner) * 0.5f;
    const float rDimTxt = outerR * (kDimOuter + kDimInner) * 0.5f;

    // Font sizes scaled to circle
    const float majFontSz = juce::jlimit(9.0f, 18.0f, outerR * 0.105f);
    const float minFontSz = juce::jlimit(7.0f, 13.0f, outerR * 0.082f);
    const float dimFontSz = juce::jlimit(6.0f, 10.0f, outerR * 0.062f);

    // Text box widths: limit to 90% of the arc chord width at each ring's midpoint,
    // so labels don't visually bleed into adjacent segments.
    const float arcSinHalf = std::sin(segAngle * 0.5f);
    const float majLabelW  = juce::jmin(56.0f, rMajTxt * 2.0f * arcSinHalf * 0.88f);
    const float minLabelW  = juce::jmin(48.0f, rMinTxt * 2.0f * arcSinHalf * 0.88f);
    const float dimLabelW  = juce::jmin(38.0f, rDimTxt * 2.0f * arcSinHalf * 0.88f);

    // ── Pass 1: all fills and borders ───────────────────────────────────
    for (int i = 0; i < 12; ++i)
    {
        // fa1/fa2 for arc drawing (JUCE convention, offset by +halfPi vs label angles)
        const float fa1 = arcStartAngle + i * segAngle + gap;
        const float fa2 = arcStartAngle + (i + 1) * segAngle - gap;

        // Major (outer) ring
        g.setColour(segmentColour(i, 0, key, scale));
        g.fillPath(makeSegment(fa1, fa2, rMajI, rMajO));
        g.setColour(kSep);
        g.strokePath(makeSegment(fa1, fa2, rMajI, rMajO), juce::PathStrokeType(0.8f));

        // Minor (middle) ring
        g.setColour(segmentColour(i, 1, key, scale));
        g.fillPath(makeSegment(fa1, fa2, rMinI, rMinO));
        g.setColour(kSep);
        g.strokePath(makeSegment(fa1, fa2, rMinI, rMinO), juce::PathStrokeType(0.6f));

        // Dim (inner) ring
        g.setColour(segmentColour(i, 2, key, scale));
        g.fillPath(makeSegment(fa1, fa2, rDimI, rDimO));
        g.setColour(kSep);
        g.strokePath(makeSegment(fa1, fa2, rDimI, rDimO), juce::PathStrokeType(0.5f));
    }

    // Centre hole drawn before labels so it doesn't overdraw the innermost text
    g.setColour(kBg);
    g.fillEllipse(centre.x - rDimI, centre.y - rDimI, rDimI * 2.0f, rDimI * 2.0f);

    // ── Pass 2: all labels (drawn on top of all fills) ───────────────────
    for (int i = 0; i < 12; ++i)
    {
        const float a1 = startAngle + i * segAngle + gap;
        const float a2 = startAngle + (i + 1) * segAngle - gap;

        // Helper: given a pc and which ring we're on, return the chord-tone text colour
        // (or juce::Colour() if this ring shouldn't be treated as a chord-tone hint).
        // ring 0 also carries the out-of-scale warning.
        auto chordToneText = [&](int pc, int r) -> juce::Colour
        {
            if (activeRoot < 0 || !isChordNote(pc)) return {};
            const bool dMaj = isDiatonicK(pc, 0, key, scale);
            const bool dMin = isDiatonicK(pc, 1, key, scale);
            const bool dDim = isDiatonicK(pc, 2, key, scale);
            if (r == 0 && dMaj) return kTextDiatonic;
            if (r == 1 && dMin) return kTextDiatonic;
            if (r == 2 && dDim) return kTextDiatonic;
            if (r == 0 && !dMaj && !dMin && !dDim) return kTextOutOfScale;
            return {};
        };

        // Major label
        {
            const int pc   = kPos[i].majorRoot;
            bool active    = isActive(i, 0);
            auto ctt       = chordToneText(pc, 0);
            bool tonic     = !active && ctt == juce::Colour() && (activeRoot < 0) && (pc == key) && kTonicRing[scale] == 0;
            bool diatonic  = !active && ctt == juce::Colour() && !tonic && isDiatonicK(pc, 0, key, scale);
            bool scaleTone = !active && ctt == juce::Colour() && !diatonic && isScaleToneOnlyK(pc, key, scale);
            juce::Colour txtCol = active               ? kTextActive
                                : ctt != juce::Colour() ? ctt
                                : tonic                ? kTextTonic
                                : diatonic             ? kTextDiatonic
                                : scaleTone            ? kTextScaleTone
                                :                        kText[0];
            drawSegmentLabel(g, kPos[i].majorName,
                             arcMidpoint(a1, a2, rMajTxt),
                             majFontSz, true, txtCol, majLabelW);
        }

        // Minor label
        {
            const int pc   = kPos[i].minorRoot;
            bool active    = isActive(i, 1);
            auto ctt       = chordToneText(pc, 1);
            bool tonic     = !active && ctt == juce::Colour() && (activeRoot < 0) && (pc == key) && kTonicRing[scale] == 1;
            bool diatonic  = !active && ctt == juce::Colour() && !tonic && isDiatonicK(pc, 1, key, scale);
            bool scaleTone = !active && ctt == juce::Colour() && !diatonic && isScaleToneOnlyK(pc, key, scale);
            juce::Colour txtCol = active               ? kTextActive
                                : ctt != juce::Colour() ? ctt
                                : tonic                ? kTextTonic
                                : diatonic             ? kTextDiatonic
                                : scaleTone            ? kTextScaleTone
                                :                        kText[1];
            drawSegmentLabel(g, kPos[i].minorName,
                             arcMidpoint(a1, a2, rMinTxt),
                             minFontSz, false, txtCol, minLabelW);
        }

        // Dim label
        {
            const int pc   = kPos[i].dimRoot;
            bool active    = isActive(i, 2);
            auto ctt       = chordToneText(pc, 2);
            bool tonic     = !active && ctt == juce::Colour() && (activeRoot < 0) && (pc == key) && kTonicRing[scale] == 2;
            bool diatonic  = !active && ctt == juce::Colour() && !tonic && isDiatonicK(pc, 2, key, scale);
            bool scaleTone = !active && ctt == juce::Colour() && !diatonic && isScaleToneOnlyK(pc, key, scale);
            juce::Colour txtCol = active               ? kTextActive
                                : ctt != juce::Colour() ? ctt
                                : tonic                ? kTextTonic
                                : diatonic             ? kTextDiatonic
                                : scaleTone            ? kTextScaleTone
                                :                        kText[2];
            drawSegmentLabel(g, kPos[i].dimName,
                             arcMidpoint(a1, a2, rDimTxt),
                             dimFontSz, false, txtCol, dimLabelW);
        }
    }

    // Subtle ring outlines
    g.setColour(kSep.withAlpha(0.5f));
    for (float r : { rMajO, rMajI, rMinI, rDimI })
        g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.6f);

}
