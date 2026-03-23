# Begonia — Orchid-Style Chord Instrument Plugin

A VST3/AU plugin for macOS inspired by the [Telepathic Instruments Orchid](https://telepathicinstruments.com/), a left-hand/right-hand chord instrument. Send a root note via MIDI, hold computer keyboard keys to select chord type and extensions, and Begonia voices and plays the full chord — through its built-in synth and/or out as MIDI.

---

## Download

Get the latest pre-built binaries from the [**Releases**](../../releases/latest) page.

| File | For |
|------|-----|
| `Begonia-VST3-macOS.zip` | VST3 — Ableton, Reaper, Bitwig, etc. |
| `Begonia-AU-macOS.zip` | Audio Unit — Logic Pro, GarageBand |
| `Begonia-Standalone-macOS.zip` | Standalone app — no DAW needed |

Builds target Apple Silicon (arm64). Intel Macs run these natively via Rosetta 2 — no extra steps needed.

**System requirements:** macOS 12 or later.

---

## Installation

1. Unzip the downloaded file.
2. Copy to the appropriate location:
   - **VST3** → `~/Library/Audio/Plug-Ins/VST3/`
   - **AU** → `~/Library/Audio/Plug-Ins/Components/`
   - **Standalone** → `/Applications/` (or anywhere you like)
3. **Bypass Gatekeeper** (required for unsigned builds): right-click the file → **Open**, then confirm in the dialog. You only need to do this once per file.
4. Rescan plugins in your DAW (not needed for Standalone).

For AU, run this after copying to force CoreAudio to detect the new component:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null; sleep 1
```

---

## How It Works

Begonia separates chord playing into two hands:

- **Right hand** — plays the **root note** on a MIDI keyboard
- **Left hand** — holds **computer keyboard keys** to select chord type and extensions

As long as a MIDI note is held, changing any keyboard key immediately re-voices the chord in real time.

---

## Keyboard Mapping

### Chord Type (hold one)

| Key | Chord |
|-----|-------|
| `1` | Major |
| `2` | Minor |
| `3` | Sus4 |
| `4` | Diminished |

### Extensions (hold any combination)

| Key | Extension |
|-----|-----------|
| `Q` | Add 6th |
| `W` | Add m7 (dominant 7th) |
| `E` | Add Maj7 |
| `R` | Add 9th |

> `W` and `E` are mutually exclusive — a chord can't have both a dominant 7th and a major 7th.

**Example:** Hold `2` + `W` + `R`, then play C on your MIDI keyboard → **Cm9** (C minor with dominant 7th and 9th)

**Example:** Hold `1` + `E`, then play G → **Gmaj7**

---

## Key Mode

When **Key Mode** is enabled (toggle in the Voicing panel), you don't need to hold chord type keys at all. The plugin automatically determines the correct chord quality from the root note based on the selected key and scale:

- In **C Major**: playing A → Am, playing G → G, playing B → Bdim
- In **A Minor**: playing A → Am, playing C → C, playing E → Em
- Non-diatonic notes pass through as single notes

Extensions (Q/W/E/R) still apply on top of the auto-determined chord type.

Select the key and scale using the two dropdowns in the **Circle of Fifths** panel (top-right of the UI).

---

## Circle of Fifths Display

The circle in the top-right shows harmonic context at a glance:

- **Outer ring** — major chords
- **Middle ring** — minor chords
- **Inner ring** — diminished chords

Color coding (based on the selected key/scale):

| Color | Meaning |
|-------|---------|
| Teal / Blue / Purple | Diatonic chord (in key) |
| Brown | Tonic chord of the selected scale |
| Orange / Blue / Purple (bright) | Currently playing chord |
| Red | Currently playing chord is outside the scale |
| Dark amber | Chord tones on their diatonic ring |
| Dark red | Chord tone not in the selected scale |
| Faint blue | Scale degree with no standard triad |

---

## Voicing Options

Select in the **Voicing** panel (collapsible, center-left area):

| Voicing | Description |
|---------|-------------|
| Close | Root position, all notes within one octave |
| Inversion 1 | 3rd in the bass |
| Inversion 2 | 5th in the bass |
| Inversion 3 | 7th in the bass (4-note chords only) |
| Open | Alternating notes raised an octave |
| Drop 2 | Classic jazz voicing: 2nd-highest note dropped an octave |
| Spread | Root + 5th low, 3rd + 7th high — wide, cinematic |

**Octave offset** shifts the entire chord up or down by 1–2 octaves.

**Bass** toggle sends the lowest note to a separate MIDI channel (channel 2 by default) for independent bass routing.

---

## Built-in Synth Engines

Select in the **Synth** panel (center-right). ADSR and filter controls apply to all engines.

| Engine | Character |
|--------|-----------|
| Piano | Sine wave + harmonics, fast attack, percussive decay |
| Pad | Two detuned oscillators, slow attack, chorus + reverb |
| Synth | Bandlimited sawtooth, filter sweep tied to velocity |
| Strings | Slow attack, smooth release, ensemble-style |
| Pluck | Sharp transient, quick decay |
| Organ | Sustained, no release tail |

Disable synth audio entirely with the **Synth Audio** toggle — useful when using Begonia purely as a MIDI processor feeding an external instrument.

---

## MIDI Output

Begonia outputs voiced chord MIDI regardless of whether the built-in synth is enabled. Route this to any instrument in your DAW.

Configure in the **MIDI Config** bar at the bottom:

- **Chord Ch** — MIDI channel for chord notes (default: 1)
- **Bass Ch** — MIDI channel for the bass note (default: 2)
- **MIDI Out** toggle — enable/disable MIDI output

---

## Global Keys (macOS)

The **Global Keys** toggle (top-right of the UI) enables macOS-level key monitoring so keyboard input is captured even when the plugin window doesn't have focus.

This uses `NSEvent addGlobalMonitorForEventsMatchingMask:` — no Accessibility permissions required.

---

## Build from Source

See [BUILD.md](BUILD.md) for full instructions. Quick start:

```bash
git clone https://github.com/nearnshaw/VST-Orchid-like-plugin.git
cd VST-Orchid-like-plugin
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Begonia_VST3 Begonia_AU -j$(sysctl -n hw.logicalcpu)
```

Output:
- `build/Begonia_artefacts/VST3/Begonia.vst3`
- `build/Begonia_artefacts/AU/Begonia.component`

---

## License

[GPL v3](LICENSE)
