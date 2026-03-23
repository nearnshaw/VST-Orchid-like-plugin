# Building the Orchid Plugin

## Prerequisites

- macOS 12+
- Xcode 14+ (with Command Line Tools)
- CMake 3.22+
- Git

Install CMake via Homebrew if needed:
```bash
brew install cmake
```

## Build Steps

```bash
# From the project root:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j$(sysctl -n hw.logicalcpu)
```

For a Debug build (faster compilation, no optimization):
```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -- -j$(sysctl -n hw.logicalcpu)
```

## Output locations

After a successful build:

- **VST3**: `build/Orchid_artefacts/Release/VST3/Orchid.vst3`
- **AU**: `build/Orchid_artefacts/Release/AU/Orchid.component`
- **Standalone**: `build/Orchid_artefacts/Release/Standalone/Orchid.app`

## Installing

Copy the plugin to your system plugin folder:

```bash
# VST3 (Ableton, Reaper, etc.)
cp -r build/Orchid_artefacts/Release/VST3/Orchid.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (Logic Pro, GarageBand)
cp -r build/Orchid_artefacts/Release/AU/Orchid.component ~/Library/Audio/Plug-Ins/Components/

# Re-register AU with CoreAudio
killall -9 AudioComponentRegistrar 2>/dev/null || true
```

Then rescan plugins in your DAW.

## Running Tests

```bash
cmake --build build --target OrchidTests
cd build && ctest --output-on-failure
```

Or run directly:
```bash
./build/tests/OrchidTests
```

## Validating the Plugin

```bash
# VST3 validation (requires pluginval — install from https://github.com/Tracktion/pluginval)
pluginval --strictnessLevel 10 --validate build/Orchid_artefacts/Release/VST3/Orchid.vst3

# AU validation (built-in macOS tool)
auval -v aumu Orch Orcd
```

## How to Use

### Keyboard Mapping

| Key | Function |
|-----|----------|
| `1` | Major chord type |
| `2` | Minor chord type |
| `3` | Sus4 chord type |
| `4` | Diminished chord type |
| `Q` | Add 6th extension |
| `W` | Add m7 (dominant 7th) |
| `E` | Add Maj7 |
| `R` | Add 9th |

**Workflow:** Send a MIDI note (root note) to the plugin, then hold a chord type key + optional extension keys.

Example: Send MIDI C4 + hold `1` + `E` → CMaj7 chord sounds and outputs.

### Global Key Capture

Enable the "Global Keys" toggle in the UI header to capture keyboard input even when the plugin window doesn't have focus. Uses macOS NSEvent monitoring (no Accessibility permission needed).

### MIDI Routing

The plugin outputs chord notes on Ch1 and the bass note on Ch2 (configurable in the MIDI Config bar). Route these to any synth plugin for custom sounds.

### Voicing Options

- **Close**: Standard root position
- **Inversion 1/2/3**: Rotates chord tones (3rd/5th/7th in bass)
- **Open**: Alternating notes raised an octave
- **Drop 2**: Classic jazz voicing
- **Spread**: Wide cinematic voicing

### Synth Engines

- **Piano**: Sine wave with harmonics, percussive
- **Pad**: Detuned oscillators, slow attack, chorus + reverb
- **Synth**: Sawtooth wave, filter sweep with velocity
