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
    {  0,  9, 11, "C",  "Am",  "B°"  },   // 0  – key of C
    {  7,  4,  6, "G",  "Em",  "F#°" },   // 1  – key of G
    {  2, 11,  1, "D",  "Bm",  "C#°" },   // 2  – key of D
    {  9,  6,  8, "A",  "F#m", "G#°" },   // 3  – key of A
    {  4,  1,  3, "E",  "C#m", "D#°" },   // 4  – key of E
    { 11,  8, 10, "B",  "G#m", "A#°" },   // 5  – key of B
    {  6,  3,  5, "F#", "D#m", "F°"  },   // 6  – key of F#
    {  1, 10,  0, "C#", "A#m", "C°"  },   // 7  – key of C#
    {  8,  5,  7, "G#", "Fm",  "G°"  },   // 8  – key of G#
    {  3,  0,  2, "D#", "Cm",  "D°"  },   // 9  – key of D#
    { 10,  7,  9, "A#", "Gm",  "A°"  },   // 10 – key of A#
    {  5,  2,  4, "F",  "Dm",  "E°"  },   // 11 – key of F
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
    juce::Colour(0xFF183828),   // major – teal, clearly distinct from base
    juce::Colour(0xFF142038),   // minor – steel blue, clearly distinct from base
    juce::Colour(0xFF281430),   // dim   – purple, clearly distinct from base
};
const juce::Colour CircleOfFifthsDisplay::kTonic         = juce::Colour(0xFF2C2010);
const juce::Colour CircleOfFifthsDisplay::kActive[3]     = {
    juce::Colour(0xFFE06B3A),   // major – orange accent
    juce::Colour(0xFF4A7AC8),   // minor – blue
    juce::Colour(0xFF9045B5),   // dim   – purple
};
const juce::Colour CircleOfFifthsDisplay::kNoteHint      = juce::Colour(0xFF3A2C18);
const juce::Colour CircleOfFifthsDisplay::kSep           = juce::Colour(0xFF26263E);
const juce::Colour CircleOfFifthsDisplay::kText[3]       = {
    juce::Colour(0xFFCCCCE0),   // major text – light, readable on dark bg
    juce::Colour(0xFFBBBBD8),   // minor text
    juce::Colour(0xFFAAAACC),   // dim text
};
const juce::Colour CircleOfFifthsDisplay::kTextActive    = juce::Colours::white;
const juce::Colour CircleOfFifthsDisplay::kTextDiatonic  = juce::Colour(0xFF88DDAA);
const juce::Colour CircleOfFifthsDisplay::kTextTonic     = juce::Colour(0xFFE0A855);

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------
CircleOfFifthsDisplay::CircleOfFifthsDisplay(juce::AudioProcessorValueTreeState& apvts)
{
    // Key dropdown (all sharps, C=0 … B=11)
    static const char* keyNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(keyNames[i], i + 1);   // item IDs are 1-indexed
    keyCombo.setSelectedId(1, juce::dontSendNotification);

    keyAtt = std::make_unique<ComboAtt>(apvts, ParamIDs::SelectedKey, keyCombo);

    // Repaint the circle whenever the key selection changes
    keyCombo.onChange = [this]() { repaint(); };

    keyLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    keyLabel.setColour(juce::Label::textColourId, kText[0].withAlpha(0.7f));
    keyLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(keyLabel);
    addAndMakeVisible(keyCombo);
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

    // Key selector strip at top
    const int selectorH = 26;
    const int labelW    = 36;
    const int comboW    = 72;
    keyLabel.setBounds(w / 2 - labelW - comboW / 2 - 4, pad, labelW, selectorH);
    keyCombo.setBounds(w / 2 - comboW / 2, pad, comboW, selectorH);

    // Circle area below selector
    const int circleY = pad + selectorH + pad;
    const int circleH = h - circleY - pad;
    const int circleW = w - pad * 2;
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
    // keyCombo item ID is (pitchClass + 1)
    return juce::jlimit(0, 11, keyCombo.getSelectedId() - 1);
}

bool CircleOfFifthsDisplay::isDiatonic(int pc, int ring) const
{
    const int K = selectedKey();
    if (ring == 0)   // major: I (K), IV (K+5), V (K+7)
        return pc == K % 12 || pc == (K + 5) % 12 || pc == (K + 7) % 12;
    if (ring == 1)   // minor: ii (K+2), iii (K+4), vi (K+9)
        return pc == (K + 2) % 12 || pc == (K + 4) % 12 || pc == (K + 9) % 12;
    if (ring == 2)   // dim: vii° (K+11)
        return pc == (K + 11) % 12;
    return false;
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
// Colour selection (priority: active > note-hint > diatonic > base)
// -----------------------------------------------------------------------
juce::Colour CircleOfFifthsDisplay::segmentColour(int pos, int ring) const
{
    if (isActive(pos, ring))
        return kActive[ring];

    const int pc = (ring == 0) ? kPos[pos].majorRoot
                 : (ring == 1) ? kPos[pos].minorRoot
                 :               kPos[pos].dimRoot;

    // Note-hint only on the outer (major) ring
    if (ring == 0 && activeRoot >= 0 && isChordNote(pc))
        return kNoteHint;

    // Tonic chord (I) gets its own subtle accent — only major ring, no active chord playing
    if (ring == 0 && activeRoot < 0 && pc == selectedKey())
        return kTonic;

    if (isDiatonic(pc, ring))
        return kDiatonic[ring];

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
    // Small gap between segments for readability
    const float gap      = 0.012f;

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

    for (int i = 0; i < 12; ++i)
    {
        const float a1 = -halfPi + i * segAngle + gap;
        const float a2 = -halfPi + (i + 1) * segAngle - gap;

        // ── Major (outer) ring ──────────────────────────────────────
        {
            juce::Colour fill = segmentColour(i, 0);
            g.setColour(fill);
            g.fillPath(makeSegment(a1, a2, rMajI, rMajO));

            g.setColour(kSep);
            g.strokePath(makeSegment(a1, a2, rMajI, rMajO),
                         juce::PathStrokeType(0.8f));

            bool active   = isActive(i, 0);
            bool tonic    = !active && (activeRoot < 0) && (kPos[i].majorRoot == selectedKey());
            bool diatonic = !active && !tonic && isDiatonic(kPos[i].majorRoot, 0);
            juce::Colour txtCol = active ? kTextActive
                                : tonic    ? kTextTonic
                                : diatonic ? kTextDiatonic
                                : kText[0];
            drawSegmentLabel(g, kPos[i].majorName,
                             arcMidpoint(a1, a2, rMajTxt),
                             majFontSz, true, txtCol, majLabelW);
        }

        // ── Minor (middle) ring ──────────────────────────────────────
        {
            juce::Colour fill = segmentColour(i, 1);
            g.setColour(fill);
            g.fillPath(makeSegment(a1, a2, rMinI, rMinO));

            g.setColour(kSep);
            g.strokePath(makeSegment(a1, a2, rMinI, rMinO),
                         juce::PathStrokeType(0.6f));

            bool active   = isActive(i, 1);
            bool diatonic = !active && isDiatonic(kPos[i].minorRoot, 1);
            juce::Colour txtCol = active ? kTextActive
                                : diatonic ? kTextDiatonic
                                : kText[1];
            drawSegmentLabel(g, kPos[i].minorName,
                             arcMidpoint(a1, a2, rMinTxt),
                             minFontSz, false, txtCol, minLabelW);
        }

        // ── Dim (inner) ring ─────────────────────────────────────────
        {
            juce::Colour fill = segmentColour(i, 2);
            g.setColour(fill);
            g.fillPath(makeSegment(a1, a2, rDimI, rDimO));

            g.setColour(kSep);
            g.strokePath(makeSegment(a1, a2, rDimI, rDimO),
                         juce::PathStrokeType(0.5f));

            bool active   = isActive(i, 2);
            bool diatonic = !active && isDiatonic(kPos[i].dimRoot, 2);
            juce::Colour txtCol = active ? kTextActive
                                : diatonic ? kTextDiatonic
                                : kText[2];
            drawSegmentLabel(g, kPos[i].dimName,
                             arcMidpoint(a1, a2, rDimTxt),
                             dimFontSz, false, txtCol, dimLabelW);
        }
    }

    // Centre hole
    g.setColour(kBg);
    g.fillEllipse(centre.x - rDimI, centre.y - rDimI, rDimI * 2.0f, rDimI * 2.0f);

    // Optional: subtle ring outlines
    g.setColour(kSep.withAlpha(0.5f));
    for (float r : { rMajO, rMajI, rMinI, rDimI })
        g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.6f);
}
